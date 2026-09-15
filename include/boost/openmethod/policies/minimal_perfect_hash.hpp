// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICY_MINIMAL_PERFECT_HASH_HPP
#define BOOST_OPENMETHOD_POLICY_MINIMAL_PERFECT_HASH_HPP

#include <boost/openmethod/preamble.hpp>

#include <boost/assert.hpp>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#ifdef _MSC_VER
#pragma warning(push)
// 4702: unreachable code. The `abort()` after a call to the error handler is
// there for a handler that returns - the default one prints and returns - but a
// handler that is [[noreturn]], like throw_error_handler, makes it dead code,
// and MSVC diagnoses that. Same reason as in preamble.hpp and core.hpp.
#pragma warning(disable : 4702)
#endif

namespace boost::openmethod::policies {

//! Map type ids to a dense index with a minimal perfect hash.
//!
//! `minimal_perfect_hash` implements the @ref type_hash policy by hash and
//! displace, after Belazzougui, Botelho and Dietzfelbinger, without the
//! compression step:
//!
//! @code
//! h = x * seed;                          // one multiply
//! p = pilots[h >> bucket_shift];         // this bucket's pilot
//! index = mulhi(h * p, slots);           // displace, then reduce
//! @endcode
//!
//! The type ids are split into `n / Lambda` buckets by the top bits of `h`.
//! Buckets are placed largest first; each is given a 32-bit odd *pilot*, found
//! by trial, such that multiplying the key by it sends every id in the bucket
//! to a slot that is still free. The final reduction is a multiply-shift - the
//! top half of a 64x64 product - which maps onto `[0, slots)` for any `slots`
//! without a division.
//!
//! **Choose it when the table size matters more than the last nanosecond.**
//! Unlike @ref fast_perfect_hash, whose table is sized by the *distribution*
//! of the type ids and whose randomized search can fail outright on a large,
//! sparse set, this one spends `8 * n * 100 / LoadPercent` bytes of v-table
//! vector plus `4 * n / Lambda` bytes of pilots **whatever the addresses are**,
//! and its search time depends only on how many type ids there are, not where
//! they sit. That makes it the policy to reach for in a program that `dlopen`s
//! modules registering classes of their own, where type ids from different
//! modules are far apart and in unrelated ranges.
//!
//! The price is on the dispatch path: the pilot must be loaded before the index
//! can be formed, so the v-table lookup becomes two dependent loads instead of
//! one. Expect it to cost a nanosecond or two per call relative to
//! @ref fast_perfect_hash.
//!
//! It needs no instruction-set extension - a 64-bit multiply and a shift exist
//! everywhere - and makes no assumption about the layout of the type ids, so
//! unlike @ref minimal_cover_hash it is available on every target, and unlike
//! a scheme keyed on address arithmetic it does not depend on
//! @ref std_rtti.
//!
//! @note **A type id of zero is outside this policy's domain.** Zero is a fixed
//! point of a multiply, so it lands in slot 0 for every seed and every pilot;
//! it cannot be displaced, and the search fails whenever another bucket has
//! taken that slot. Addresses are never zero, so @ref std_rtti and
//! @ref static_rtti are unaffected; a custom @ref rtti policy that hands out
//! small integers must not use zero as one of them. When the registry has
//! @ref runtime_checks, @ref initialize asserts that none of the registered
//! type ids is zero.
//!
//! `LoadPercent = 100` asks for an exactly minimal table. It is reachable, but
//! not in bounded time at a large `Lambda`: the last buckets have to hit the
//! last few free slots, and the expected number of trials for a bucket of size
//! `s` facing a fraction `phi` of free slots grows as `phi^-s`. A few percent
//! of slack removes that tail. Note also that an exactly minimal table is not
//! the smallest *total*: reaching it needs the buckets halved, which doubles
//! the pilot array, and that costs more than the slots it recovers.
//!
//! @tparam Lambda Average bucket size. Larger means a smaller pilot table and
//!   a longer search.
//! @tparam LoadPercent Slots per 100 type ids, inverted: 100 is an exactly
//!   minimal table, 95 leaves one slot free in twenty.
//! @tparam MaxSeeds How many multipliers to try before reporting
//!   @ref search_error. Raising it rarely helps on its own - a set that
//!   defeats one multiplier usually defeats them all at that `Lambda` and
//!   `LoadPercent`; lower `Lambda` or `LoadPercent` instead.
//!
//! @par Example
//! include:policies.cpp#minimal_perfect_hash
//!
//! @see [Registries and Policies](xref:ROOT:registries_and_policies.adoc)
template<
    std::size_t Lambda = 4, std::size_t LoadPercent = 95,
    std::size_t MaxSeeds = 16>
struct minimal_perfect_hash : type_hash {

    //! No seed yielded a complete assignment.
    struct search_error : openmethod_error {
        //! Number of registered type ids.
        std::size_t classes;
        //! Number of multipliers tried.
        std::size_t seeds;

        template<class Registry, class Stream>
        auto write(Stream& os) const -> void;
    };

    using errors = std::variant<search_error>;

    static_assert(Lambda > 0);
    static_assert(LoadPercent > 0 && LoadPercent <= 100);

    //! `state` layout when runtime checks are disabled.
    struct no_checks {
        //! The multiplier.
        std::uint64_t seed;
        //! `64 - log2(buckets)`.
        std::size_t bucket_shift;
        //! The number of slots.
        std::uint64_t size;
        //! One 32-bit pilot per bucket.
        std::vector<std::uint32_t> pilots;
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
                minimal_perfect_hash<Lambda, LoadPercent, MaxSeeds>>();
        }

        static void check(std::size_t index, type_id type);

        // The pilot tried at step `k`. Multiplying by an odd constant is a
        // bijection on 32 bits, so the sequence walks the whole range; the
        // low bit is set because an even pilot loses the key's high bits.
        static auto pilot_at(std::uint32_t k) -> std::uint32_t {
            return (k * 0x9e3779b9u) | 1u;
        }

        // The top half of a 64x64 product.
        static auto mulhi(std::uint64_t a, std::uint64_t b) -> std::uint64_t {
#if defined(__SIZEOF_INT128__)
            return std::uint64_t((static_cast<__uint128_t>(a) * b) >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
            return __umulh(a, b);
#else
            auto lo = [](std::uint64_t v) { return v & 0xffffffffull; };
            auto hi = [](std::uint64_t v) { return v >> 32; };
            auto ll = lo(a) * lo(b);
            auto lh = lo(a) * hi(b);
            auto hl = hi(a) * lo(b);
            auto hh = hi(a) * hi(b);
            auto mid = hi(ll) + lo(lh) + lo(hl);

            return hh + hi(lh) + hi(hl) + hi(mid);
#endif
        }

        // Where a pilot sends a key.
        static auto place(
            std::uint64_t h, std::uint32_t pilot, std::uint64_t slots)
            -> std::size_t {
            return std::size_t(mulhi(h * pilot, slots));
        }

        static auto mix_seed(std::uint64_t z) -> std::uint64_t {
            z += 0x9e3779b97f4a7c15ull;
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;

            return (z ^ (z >> 31)) | 1;
        }

        static auto build(
            const std::vector<std::uint64_t>& ids, std::uint64_t seed,
            std::size_t bucket_shift, std::uint64_t slots,
            std::vector<std::uint32_t>& pilots) -> bool;

      public:
        //! Finds a seed and the pilots.
        //!
        //! @tparam Context An @ref InitializeContext.
        //! @param ctx A Context object.
        //! @param options A tuple of option objects.
        template<class Context, class... Options>
        static auto initialize(
            const Context& ctx, const std::tuple<Options...>& options) -> void;

        //! Returns the hash range: `[0, slots - 1]`.
        static auto hash_range() -> std::pair<std::size_t, std::size_t> {
            return std::pair{
                std::size_t(0), std::size_t(st().size ? st().size - 1 : 0)};
        }

        //! Map a type id to an index
        //!
        //! @param type The type_id to map
        //! @return The index
        BOOST_FORCEINLINE
        static auto hash(type_id type) -> std::size_t {
            auto h = std::uint64_t(reinterpret_cast<detail::uintptr>(type)) *
                st().seed;
            auto pilot = st().pilots[std::size_t(h >> st().bucket_shift)];
            auto index = place(h, pilot, st().size);

            if constexpr (Registry::has_runtime_checks) {
                check(index, type);
            }

            return index;
        }

        //! Releases the pilot table, and the control table if there is one.
        template<class... Options>
        static auto finalize(const std::tuple<Options...>& options) -> void {
            (void)options;

            st().pilots.clear();
            st().pilots.shrink_to_fit();
            st().size = 0;

            if constexpr (Registry::has_runtime_checks) {
                st().control.clear();
                st().control.shrink_to_fit();
            }
        }
    };
};

template<std::size_t Lambda, std::size_t LoadPercent, std::size_t MaxSeeds>
template<class Registry>
auto minimal_perfect_hash<Lambda, LoadPercent, MaxSeeds>::fn<Registry>::build(
    const std::vector<std::uint64_t>& ids, std::uint64_t seed,
    std::size_t bucket_shift, std::uint64_t slots,
    std::vector<std::uint32_t>& pilots) -> bool {
    auto n = ids.size();
    auto buckets = pilots.size();

    // Bucket each id, and keep the hashed key the pilot will displace.
    std::vector<std::uint64_t> keys(n);
    std::vector<std::uint32_t> bucket_of(n);

    for (std::size_t i = 0; i != n; ++i) {
        auto h = ids[i] * seed;
        bucket_of[i] = std::uint32_t(h >> bucket_shift);
        keys[i] = h;
    }

    // Group by bucket: counting sort into a CSR-style pair of arrays.
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

    // The last buckets face a nearly full table. A bucket of size `s` with a
    // fraction `phi` of the slots free needs about `phi^-s` trials, so the
    // budget has to be generous; it is a diagnostic, not a working limit.
    const std::uint32_t max_tries = 1u << 20;

    std::vector<char> occupied(std::size_t(slots), 0);
    std::vector<std::size_t> placed;
    placed.reserve(64);

    for (auto b : order) {
        auto first = start[b], last = start[b + 1];

        if (first == last) {
            pilots[b] = 0;

            continue;
        }

        bool done = false;

        for (std::uint32_t k = 0; k != max_tries; ++k) {
            auto pilot = pilot_at(k);
            placed.clear();
            bool ok = true;

            for (auto m = first; m != last; ++m) {
                auto index = place(keys[members[m]], pilot, slots);

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

                pilots[b] = pilot;
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

template<std::size_t Lambda, std::size_t LoadPercent, std::size_t MaxSeeds>
template<class Registry>
template<class Context, class... Options>
auto minimal_perfect_hash<Lambda, LoadPercent, MaxSeeds>::fn<Registry>::
    initialize(const Context& ctx, const std::tuple<Options...>& options)
        -> void {
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

    // Zero is a fixed point of the multiply: it lands in slot 0 for every seed and every pilot,
    // so it cannot be displaced and the search fails spuriously whenever
    // another bucket has taken that slot. A type id of zero is therefore
    // outside this policy's domain - see the class documentation. `ids` is
    // sorted, so one comparison settles it.
    if constexpr (Registry::has_runtime_checks) {
        BOOST_ASSERT(ids.empty() || ids.front() != 0);
    }

    auto n = ids.size();

    if (n == 0) {
        st().seed = 1;
        st().bucket_shift = 63;
        st().size = 0;
        st().pilots.assign(2, 0);

        return;
    }

    // Slots. `LoadPercent = 100` asks for exactly one per type id.
    auto slots = (n * 100 + LoadPercent - 1) / LoadPercent;

    // Smallest power of two of at least ceil(n / Lambda) buckets, and never
    // fewer than two, so the shift stays below 64.
    std::size_t buckets = 2;
    std::size_t log_buckets = 1;

    while (buckets * Lambda < n) {
        buckets <<= 1;
        ++log_buckets;
    }

    std::vector<std::uint32_t> pilots(buckets);
    std::size_t bucket_shift = 64 - log_buckets;
    std::size_t seeds = 0;
    bool found = false;

    for (; seeds != MaxSeeds; ++seeds) {
        auto seed = mix_seed(seeds);

        if (build(ids, seed, bucket_shift, slots, pilots)) {
            st().seed = seed;
            found = true;
            ++seeds;

            break;
        }
    }

    if (!found) {
        search_error error;
        error.classes = n;
        error.seeds = seeds;

        if constexpr (Registry::has_error_handler) {
            Registry::error_handler::error(error);
        }

        abort();
    }

    st().bucket_shift = bucket_shift;
    st().size = slots;
    st().pilots = std::move(pilots);

    if constexpr (Context::template has_option<trace>) {
        ctx.tr << "  type ids: " << n << ", buckets: " << buckets
               << ", seeds tried: " << seeds << ", table: " << slots
               << " slots + " << buckets << " pilots\n";
    }

    if constexpr (Registry::has_runtime_checks) {
        st().control.assign(std::size_t(slots), type_id(detail::uintptr_max));

        for (auto id : ids) {
            auto h = id * st().seed;
            auto pilot = st().pilots[std::size_t(h >> bucket_shift)];
            st().control[place(h, pilot, slots)] =
                reinterpret_cast<type_id>(id);
        }
    }
}

template<std::size_t Lambda, std::size_t LoadPercent, std::size_t MaxSeeds>
template<class Registry>
void minimal_perfect_hash<Lambda, LoadPercent, MaxSeeds>::fn<Registry>::check(
    std::size_t index, type_id type) {
    if (index >= st().size || st().control[index] != type) {
        if constexpr (Registry::has_error_handler) {
            missing_class error;
            error.type = type;
            Registry::error_handler::error(error);
        }

        abort();
    }
}

template<std::size_t Lambda, std::size_t LoadPercent, std::size_t MaxSeeds>
template<class Registry, class Stream>
auto minimal_perfect_hash<Lambda, LoadPercent, MaxSeeds>::search_error::write(
    Stream& os) const -> void {
    os << "could not place " << classes << " type ids after trying " << seeds
       << " multipliers\n";
}

} // namespace boost::openmethod::policies

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
