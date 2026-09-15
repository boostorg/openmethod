// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICY_TWO_LEVEL_HASH_HPP
#define BOOST_OPENMETHOD_POLICY_TWO_LEVEL_HASH_HPP

#include <boost/openmethod/preamble.hpp>

#include <boost/assert.hpp>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
// 4702: unreachable code. The `abort()` after a call to the error handler is
// there for a handler that returns - the default one prints and returns - but a
// handler that is [[noreturn]], like throw_error_handler, makes it dead code,
// and MSVC diagnoses that. Same reason as in preamble.hpp and core.hpp.
#pragma warning(disable : 4702)
#endif

namespace boost::openmethod::policies {

//! Map type ids to an index with two multiply-shifts.
//!
//! `two_level_hash` implements the @ref type_hash policy with a per-bucket
//! multiplier into a shared power-of-two table:
//!
//! @code
//! h     = m1 * x;                  // mix once
//! h1(x) = h >> s1;                 // which bucket, 2^b of them
//! h2(x) = (m2[h1(x)] * h) >> s2;   // the index, into 2^t slots
//! @endcode
//!
//! The first level is one imperfect multiply-shift into a fixed number of
//! buckets; the second is a *per-bucket* multiplier, found by trial, that sends
//! every id in its bucket to a slot that is still free.
//!
//! It is @ref minimal_perfect_hash with one thing changed: where that reduces
//! with the top half of a product, onto a table of any size, this one shifts,
//! onto a table whose size is a power of two. The shift is the cheaper of the
//! two where the compiler hoists the shift amount out of the dispatch loop,
//! which is worth about a nanosecond per call; where it reloads it on every
//! call the two cost the same. Both are slower than @ref fast_perfect_hash.
//!
//! What the power-of-two table costs is a *sawtooth*: it holds
//! `2^ceil(log2(n))` slots, so between 1.0 and 2.0 per type id depending on
//! where `n` falls relative to a power of two, against a flat
//! `100 / LoadPercent` for @ref minimal_perfect_hash. Which of the two is
//! smaller is decided by the class count, which a program does not usually
//! control. Prefer @ref minimal_perfect_hash when the footprint has to be
//! predictable, and this one when the dispatch path matters more.
//!
//! The second multiply has to be applied to `h`, not to `x` itself. Two type
//! ids in one module are a few tens of bytes apart, so `m2 * x` differs between
//! them by about `m2 * 16`; for a 32-bit `m2` that is below `2^(64 - t)`, the
//! shift discards it, and the two land on the same slot for every multiplier
//! the search can try. Multiplying the already-mixed `h` costs nothing, since
//! `h` is ready long before `m2` arrives from the table.
//!
//! Like @ref minimal_perfect_hash this needs no instruction-set extension and
//! makes no assumption about the layout of the type ids.
//!
//! @note **A type id of zero is outside this policy's domain**, exactly as it is
//! for @ref minimal_perfect_hash. Zero is a fixed point of both multiplies, so
//! it lands in slot 0 whatever `m1` and the per-bucket multiplier are, and the
//! search fails whenever another bucket has taken that slot. Addresses are never zero; a custom @ref rtti policy handing out small
//! integers must not use zero. When the registry has @ref runtime_checks,
//! @ref initialize asserts that none of the registered type ids is zero.
//!
//! @tparam Lambda Average bucket size.
//! @tparam MaxDoublings How far the table may grow past the smallest power of
//!   two that could hold the type ids, before @ref search_error is reported.
//!   The cap is relative to the class count deliberately: an absolute one lets
//!   a pathological input ask for a table orders of magnitude larger than the
//!   program needs.
//!
//! @par Example
//! include:policies.cpp#two_level_hash
//!
//! @see [Registries and Policies](xref:ROOT:registries_and_policies.adoc)
template<std::size_t Lambda = 4, std::size_t MaxDoublings = 3>
struct two_level_hash : type_hash {

    //! No table within `MaxDoublings` doublings admitted a complete
    //! assignment.
    struct search_error : openmethod_error {
        //! Number of registered type ids.
        std::size_t classes;
        //! Widest table tried.
        std::size_t table_bits;

        template<class Registry, class Stream>
        auto write(Stream& os) const -> void;
    };

    using errors = std::variant<search_error>;

    static_assert(Lambda > 0);

    //! `state` layout when runtime checks are disabled.
    struct no_checks {
        //! First-level multiplier.
        std::uint64_t m1;
        //! `64 - b`.
        std::size_t s1;
        //! `64 - t`.
        std::size_t s2;
        //! Second-level multiplier, one per bucket.
        std::vector<std::uint32_t> m2;
        //! `2^t`.
        std::size_t slots;
    };

    //! `state` layout when runtime checks are enabled: adds the table of
    //! registered type ids used to validate hashed types.
    struct with_checks : no_checks {
        std::vector<type_id> control;
    };

    //! A TypeHashFn metafunction.
    //!
    //! @tparam Registry The registry containing this policy
    template<class Registry>
    class fn {
      public:
        using state = std::conditional_t<
            Registry::has_runtime_checks, with_checks, no_checks>;

      private:
        static auto& st() {
            return Registry::template state<
                two_level_hash<Lambda, MaxDoublings>>();
        }

        static void check(std::size_t index, type_id type);

        // The second-level multiplier tried at step `k`. Odd, because an even
        // multiplier throws away the key's top bits.
        static auto m2_at(std::uint32_t k) -> std::uint32_t {
            auto z = k * 0x9e3779b9u;
            z ^= z >> 15;
            z *= 0x85ebca6bu;
            z ^= z >> 13;

            return z | 1u;
        }

        static auto m1_at(std::uint64_t z) -> std::uint64_t {
            z += 0x9e3779b97f4a7c15ull;
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;

            return (z ^ (z >> 31)) | 1;
        }

        static auto build(
            const std::vector<std::uint64_t>& ids, std::uint64_t m1,
            std::size_t s1, std::size_t s2, std::size_t slots,
            std::vector<std::uint32_t>& m2) -> bool;

      public:
        //! Finds `m1` and the per-bucket multipliers.
        //!
        //! @tparam Context An @ref InitializeContext.
        //! @param ctx A Context object.
        //! @param options A tuple of option objects.
        template<class Context, class... Options>
        static auto initialize(
            const Context& ctx, const std::tuple<Options...>& options) -> void;

        //! Returns the hash range: `[0, slots - 1]`.
        static auto hash_range() -> std::pair<std::size_t, std::size_t> {
            return std::pair{std::size_t(0), st().slots ? st().slots - 1 : 0};
        }

        //! Map a type id to an index
        //!
        //! @param type The type_id to map
        //! @return The index
        BOOST_FORCEINLINE
        static auto hash(type_id type) -> std::size_t {
            auto h = std::uint64_t(reinterpret_cast<detail::uintptr>(type)) *
                st().m1;
            auto bucket = std::size_t(h >> st().s1);
            auto index =
                std::size_t((std::uint64_t(st().m2[bucket]) * h) >> st().s2);

            if constexpr (Registry::has_runtime_checks) {
                check(index, type);
            }

            return index;
        }

        //! Releases the multiplier table, and the control table if there is
        //! one.
        template<class... Options>
        static auto finalize(const std::tuple<Options...>& options) -> void {
            (void)options;

            st().m2.clear();
            st().m2.shrink_to_fit();
            st().slots = 0;

            if constexpr (Registry::has_runtime_checks) {
                st().control.clear();
                st().control.shrink_to_fit();
            }
        }
    };
};

template<std::size_t Lambda, std::size_t MaxDoublings>
template<class Registry>
auto two_level_hash<Lambda, MaxDoublings>::fn<Registry>::build(
    const std::vector<std::uint64_t>& ids, std::uint64_t m1, std::size_t s1,
    std::size_t s2, std::size_t slots, std::vector<std::uint32_t>& m2) -> bool {
    auto n = ids.size();
    auto buckets = m2.size();

    // Group by bucket: counting sort into a CSR-style pair of arrays.
    std::vector<std::uint64_t> keys(n);
    std::vector<std::uint32_t> bucket_of(n);

    for (std::size_t i = 0; i != n; ++i) {
        keys[i] = m1 * ids[i];
        bucket_of[i] = std::uint32_t(keys[i] >> s1);
    }

    std::vector<std::uint32_t> start(buckets + 1, 0);

    for (auto b : bucket_of) {
        ++start[b + 1];
    }

    std::partial_sum(start.begin(), start.end(), start.begin());
    std::vector<std::uint32_t> members(n);
    auto fill = start;

    for (std::size_t i = 0; i != n; ++i) {
        members[fill[bucket_of[i]]++] = std::uint32_t(i);
    }

    // Largest buckets first: a big bucket is placeable only while the table
    // is still mostly empty.
    std::vector<std::uint32_t> order(buckets);
    std::iota(order.begin(), order.end(), std::uint32_t(0));
    std::stable_sort(
        order.begin(), order.end(), [&](std::uint32_t a, std::uint32_t b) {
            return (start[a + 1] - start[a]) > (start[b + 1] - start[b]);
        });

    const std::uint32_t max_tries = 1u << 20;
    std::vector<char> occupied(slots, 0);
    std::vector<std::size_t> placed;
    placed.reserve(64);

    for (auto b : order) {
        auto first = start[b], last = start[b + 1];

        if (first == last) {
            m2[b] = 1;

            continue;
        }

        bool done = false;

        for (std::uint32_t k = 0; k != max_tries; ++k) {
            auto candidate = m2_at(k);
            placed.clear();
            bool ok = true;

            for (auto m = first; m != last; ++m) {
                auto index = std::size_t(
                    (std::uint64_t(candidate) * keys[members[m]]) >> s2);

                if (occupied[index] ||
                    std::find(placed.begin(), placed.end(), index) !=
                        placed.end()) {
                    ok = false;

                    break;
                }

                placed.push_back(index);
            }

            if (ok) {
                for (auto index : placed) {
                    occupied[index] = 1;
                }

                m2[b] = candidate;
                done = true;

                break;
            }
        }

        if (!done) {
            return false;
        }
    }

    return true;
}

template<std::size_t Lambda, std::size_t MaxDoublings>
template<class Registry>
template<class Context, class... Options>
auto two_level_hash<Lambda, MaxDoublings>::fn<Registry>::initialize(
    const Context& ctx, const std::tuple<Options...>& options) -> void {
    (void)options;

    std::vector<std::uint64_t> ids;

    for (auto iter = ctx.classes_begin(); iter != ctx.classes_end(); ++iter) {
        for (auto type_iter = iter->type_id_begin();
             type_iter != iter->type_id_end(); ++type_iter) {
            ids.push_back(
                std::uint64_t(reinterpret_cast<detail::uintptr>(*type_iter)));
        }
    }

    // One class may be registered under the same type id by several modules;
    // the table is over *distinct* ids.
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    // Zero is a fixed point of the multiply: it lands in slot 0 whatever `m1` and the per-bucket multiplier are,
    // so it cannot be displaced and the search fails spuriously whenever
    // another bucket has taken that slot. A type id of zero is therefore
    // outside this policy's domain - see the class documentation. `ids` is
    // sorted, so one comparison settles it.
    if constexpr (Registry::has_runtime_checks) {
        BOOST_ASSERT(ids.empty() || ids.front() != 0);
    }

    auto n = ids.size();

    if (n == 0) {
        st().m1 = 1;
        st().s1 = 63;
        st().s2 = 63;
        st().m2.assign(2, 1);
        st().slots = 0;

        return;
    }

    // Never fewer than two buckets, so the shift stays below 64.
    std::size_t buckets = 2, b = 1;

    while (buckets * Lambda < n) {
        buckets <<= 1;
        ++b;
    }

    // The first-level multiplier is taken as it comes. Scoring several and
    // keeping the one that spreads the buckets most evenly was measured, and
    // does not pay: the placement cost is set by the tail, where every
    // remaining bucket faces an almost full table, and evening the bucket
    // sizes does not change how full that table is.
    auto m1 = m1_at(0);

    // Smallest power-of-two table that can hold them, grown on failure.
    std::size_t t = 1;

    while ((std::size_t(1) << t) < n) {
        ++t;
    }

    std::vector<std::uint32_t> m2(buckets);
    bool found = false;

    const std::size_t max_t = t + MaxDoublings;

    for (; t <= max_t; ++t) {
        if (build(ids, m1, 64 - b, 64 - t, std::size_t(1) << t, m2)) {
            found = true;

            break;
        }
    }

    if (!found) {
        search_error error;
        error.classes = n;
        error.table_bits = max_t;

        if constexpr (Registry::has_error_handler) {
            Registry::error_handler::error(error);
        }

        abort();
    }

    st().m1 = m1;
    st().s1 = 64 - b;
    st().s2 = 64 - t;
    st().slots = std::size_t(1) << t;
    st().m2 = std::move(m2);

    if constexpr (Context::template has_option<trace>) {
        ctx.tr << "  type ids: " << n << ", buckets: " << buckets
               << ", table: " << st().slots << " slots\n";
    }

    if constexpr (Registry::has_runtime_checks) {
        st().control.assign(st().slots, type_id(detail::uintptr_max));

        for (auto id : ids) {
            auto h = st().m1 * id;
            auto bucket = std::size_t(h >> st().s1);
            auto index =
                std::size_t((std::uint64_t(st().m2[bucket]) * h) >> st().s2);
            st().control[index] = reinterpret_cast<type_id>(id);
        }
    }
}

template<std::size_t Lambda, std::size_t MaxDoublings>
template<class Registry>
void two_level_hash<Lambda, MaxDoublings>::fn<Registry>::check(
    std::size_t index, type_id type) {
    if (index >= st().slots || st().control[index] != type) {
        if constexpr (Registry::has_error_handler) {
            missing_class error;
            error.type = type;
            Registry::error_handler::error(error);
        }

        abort();
    }
}

template<std::size_t Lambda, std::size_t MaxDoublings>
template<class Registry, class Stream>
auto two_level_hash<Lambda, MaxDoublings>::search_error::write(Stream& os) const
    -> void {
    os << "could not place " << classes << " type ids in a table of up to 2^"
       << table_bits << " slots\n";
}

} // namespace boost::openmethod::policies

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
