// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICY_MINIMAL_COVER_HASH_HPP
#define BOOST_OPENMETHOD_POLICY_MINIMAL_COVER_HASH_HPP

#include <boost/openmethod/preamble.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>

// Detect BMI2's parallel bit extract. GCC and clang define __BMI2__ when the
// instruction is enabled, which takes -mbmi2 or a -march= that implies it, and
// reject the intrinsic without it. MSVC gates nothing on a macro and emits the
// instruction from the intrinsic, so there the test is the target alone.
//
// clang-cl answers to both descriptions, and must be read as clang: it defines
// _MSC_VER and _M_X64, but `_pext_u64` is still the always_inline function that
// needs the `bmi2` target feature. Taking it for MSVC would turn this macro on
// for a compiler that then refuses the intrinsic - "always_inline function
// '_pext_u64' requires target feature 'bmi2'" - which is the error this macro
// exists to keep users away from. Hence the !defined(__clang__): clang-cl falls
// to the first arm, where it belongs, and it defines __x86_64__ as well.
//
// Either way the target must be x86-*64*. `_pext_u64` extracts from a 64-bit
// value and exists only in 64-bit mode: on 32-bit x86 there is `_pext_u32` and
// nothing wider, so a guard that accepted `_M_IX86` - or `__BMI2__` on an
// `-m32` build - would let the header reach an intrinsic that is not declared.
//
// The detection and the documented macro are separate so that the latter is one
// unconditional #define, with its doc comment directly attached. A comment
// separated from its #define by a preprocessor directive is not attached to it,
// and MrDocs then produces no page - which would make every @ref to the macro
// render as plain text.
#if (defined(__BMI2__) && defined(__x86_64__)) ||                              \
    (defined(_MSC_VER) && !defined(__clang__) && defined(_M_X64))
#define BOOST_OPENMETHOD_DETAIL_HAS_PEXT 1
#else
#define BOOST_OPENMETHOD_DETAIL_HAS_PEXT 0
#endif

//! Whether @ref boost::openmethod::policies::minimal_cover_hash can be used on
//! this target.
//!
//! 1 if the compiler can emit BMI2's parallel bit extract, `pext`, and 0
//! otherwise. The header always compiles; what fails, with a diagnostic, is
//! naming the policy in a registry when this is 0. A program that offers the
//! policy as an option guards the declaration with it:
//!
//! @code
//! #if BOOST_OPENMETHOD_HAS_PEXT
//! struct my_registry :
//!     boost::openmethod::default_registry::with<
//!         boost::openmethod::policies::minimal_cover_hash<>> {};
//! #endif
//! @endcode
//!
//! @see [Registries and Policies](xref:ROOT:registries_and_policies.adoc)
#define BOOST_OPENMETHOD_HAS_PEXT BOOST_OPENMETHOD_DETAIL_HAS_PEXT

#if BOOST_OPENMETHOD_HAS_PEXT
#include <immintrin.h>
#endif

#ifdef _MSC_VER
#pragma warning(push)
// 4702: unreachable code. The `abort()` after a call to the error handler is
// there for a handler that returns - the default one prints and returns - but a
// handler that is [[noreturn]], like throw_error_handler, makes it dead code,
// and MSVC diagnoses that. Same reason as in preamble.hpp and core.hpp.
#pragma warning(disable : 4702)
#endif

namespace boost::openmethod {

namespace detail {

// BOOST_OPENMETHOD_HAS_PEXT, made dependent on a template parameter.
//
// A static_assert whose condition does not depend on the enclosing template may
// be diagnosed as soon as the template is *defined*, rather than when it is
// instantiated - the standard calls such a template ill-formed, no diagnostic
// required, and compilers differ on when they report it. GCC 11 and 12, and
// Clang 13 through 15, report it immediately; GCC 13 and later, and Clang 18
// and later, wait for the instantiation. Written the obvious way, the assertion
// in minimal_cover_hash::fn would therefore make *including this header* an
// error on those compilers whenever the instruction is unavailable - which is
// the one thing the header promises not to do.
template<class>
inline constexpr bool has_pext = BOOST_OPENMETHOD_HAS_PEXT != 0;

// Cold path only: the cover search counts mask bits, the dispatch path does
// not. Plain C++ rather than an intrinsic, so it carries no instruction-set
// requirement of its own - minimal_cover_hash already has one, and one is
// quite enough.
inline auto popcount64(std::uint64_t bits) -> std::size_t {
#if defined(__GNUC__) || defined(__clang__)
    return std::size_t(__builtin_popcountll(bits));
#else
    bits = bits - ((bits >> 1) & 0x5555555555555555ull);
    bits =
        (bits & 0x3333333333333333ull) + ((bits >> 2) & 0x3333333333333333ull);
    bits = (bits + (bits >> 4)) & 0x0f0f0f0f0f0f0f0full;

    return std::size_t((bits * 0x0101010101010101ull) >> 56);
#endif
}

// The dispatch path's one instruction. Wrapped so that the header parses on a
// target without it: the stub is never reached, because naming the policy in a
// registry static_asserts first.
inline auto pext64(std::uint64_t value, std::uint64_t mask) -> std::uint64_t {
#if BOOST_OPENMETHOD_HAS_PEXT
    return _pext_u64(value, mask);
#else
    (void)value;
    (void)mask;

    return 0;
#endif
}

} // namespace detail

namespace policies {

//! Map type ids to indexes by extracting a minimal cover of their bits.
//!
//! `minimal_cover_hash` implements the @ref type_hash policy as
//! `H(x) = pext(x, mask)`: the bits of `x` selected by `mask`, packed into the
//! low `popcount(mask)` positions by BMI2's parallel bit extract, one
//! instruction. The index range is `[0, 2^popcount(mask))`.
//!
//! `mask` is a *minimal cover*: a smallest-found set of bit positions such that
//! `x & mask` is still injective over the registered type ids. That is exactly
//! the condition for `pext` to be injective, so the search never needs `pext`
//! itself. Unlike @ref fast_perfect_hash's randomized multiplier search it is
//! deterministic, and it finishes in milliseconds on inputs where that search
//! gives up:
//!
//! @li the bits that vary at all are trivially a cover;
//! @li a greedy pass drops bits, lowest entropy first, while injectivity holds;
//! @li a second greedy pass builds a cover bottom-up, adding the bit that
//!   resolves the most collisions each time, then trims it the same way;
//! @li the smaller of the two wins.
//!
//! **Choose it when dispatch must not get slower but the default search is a
//! problem.** One `pext` costs about what a multiply and a shift cost, so
//! dispatch is as fast as with @ref fast_perfect_hash; what this buys is a
//! table found deterministically, in bounded time. Its footprint is comparable
//! to `fast_perfect_hash`{empty}'s - both widen when the type ids are sparse,
//! and for the same reason - so it is not the policy to pick for memory.
//! @ref minimal_perfect_hash is.
//!
//! Type ids from different modules differ in many high bits, but those bits are
//! perfectly correlated, since they all encode which module. The cover keeps
//! about `log2(modules)` of them and the rest cost nothing, so a program that
//! `dlopen`{empty}s modules pays one extra bit rather than an unusable table.
//!
//! @warning **BMI2 is required, and that is not a portable requirement.** `pext`
//! requires a 64-bit x86 target - it is absent on ARM and on 32-bit x86
//! altogether, absent on x86-64 before Haswell and Excavator, and is microcoded
//! on AMD Zen 1 and Zen 2 - around 18 cycles rather than 3 - where this policy
//! will be slower than the default rather than faster. Because `hash` is
//! inlined into every dispatch, `-mbmi2` (or a `-march=` implying it) has to be
//! set for **every** translation unit of the program, and of any module sharing
//! the registry, not just one; a binary built with it executes an illegal
//! instruction on the first dispatch on a CPU that lacks `pext`. MSVC is the
//! exception: it emits the instruction from the intrinsic on any 64-bit target,
//! and takes no flag; clang-cl is not MSVC here, and wants `-mbmi2` or
//! `/arch:AVX2`. Naming this policy in a registry where
//! @ref BOOST_OPENMETHOD_HAS_PEXT is 0 is a compile error.
//! @ref minimal_perfect_hash is the portable alternative, at a cost of a
//! nanosecond or two per call.
//!
//! After "Perfect Hashing in an Imperfect World", Joaquin M. Lopez Munoz.
//!
//! @tparam MaxBits Refuse a cover wider than this; the table is `8 << bits`
//!   bytes.
//!
//! @par Example
//! include:policies.cpp#minimal_cover_hash
//!
//! @see [Registries and Policies](xref:ROOT:registries_and_policies.adoc)
template<std::size_t MaxBits = 24>
struct minimal_cover_hash : type_hash {

    //! The minimal cover found is too wide for a table.
    struct too_many_bits : openmethod_error {
        //! Number of registered type ids.
        std::size_t classes;
        //! Width of the smallest cover found.
        std::size_t bits;

        template<class Registry, class Stream>
        auto write(Stream& os) const -> void;
    };

    using errors = std::variant<too_many_bits>;

    //! `state` layout when runtime checks are disabled.
    struct no_checks {
        //! The bit positions to extract.
        std::uint64_t mask;
        //! The highest index in use.
        std::size_t max_value;
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
        static_assert(
            detail::has_pext<Registry>,
            "minimal_cover_hash needs BMI2: compile every translation unit "
            "with -mbmi2 (or a -march= that implies it), or use "
            "minimal_perfect_hash, which is portable.");

      public:
        using state = std::conditional_t<
            Registry::has_runtime_checks, with_checks, no_checks>;

      private:
        static auto& st() {
            return Registry::template state<minimal_cover_hash<MaxBits>>();
        }

        static void check(std::size_t index, type_id type);

        static auto injective(
            const std::vector<std::uint64_t>& ids, std::uint64_t mask,
            std::vector<std::uint64_t>& scratch) -> bool;
        static auto collisions(
            const std::vector<std::uint64_t>& ids, std::uint64_t mask,
            std::vector<std::uint64_t>& scratch) -> std::size_t;
        static auto minimal_cover(const std::vector<std::uint64_t>& ids)
            -> std::uint64_t;

      public:
        //! Finds the cover.
        //!
        //! @tparam Context An @ref InitializeContext.
        //! @param ctx A Context object.
        //! @param options A tuple of option objects.
        template<class Context, class... Options>
        static auto initialize(
            const Context& ctx, const std::tuple<Options...>& options) -> void;

        //! Returns the hash range: `[0, max index in use]`.
        static auto hash_range() -> std::pair<std::size_t, std::size_t> {
            return std::pair{std::size_t(0), st().max_value};
        }

        //! Map a type id to an index
        //!
        //! @param type The type_id to map
        //! @return The index
        BOOST_FORCEINLINE
        static auto hash(type_id type) -> std::size_t {
            auto index = std::size_t(
                detail::pext64(
                    static_cast<std::uint64_t>(
                        reinterpret_cast<detail::uintptr>(type)),
                    st().mask));

            if constexpr (Registry::has_runtime_checks) {
                check(index, type);
            }

            return index;
        }

        //! Releases the control table, if there is one.
        template<class... Options>
        static auto finalize(const std::tuple<Options...>& options) -> void {
            (void)options;

            st().mask = 0;
            st().max_value = 0;

            if constexpr (Registry::has_runtime_checks) {
                st().control.clear();
                st().control.shrink_to_fit();
            }
        }
    };
};

template<std::size_t MaxBits>
template<class Registry>
auto minimal_cover_hash<MaxBits>::fn<Registry>::injective(
    const std::vector<std::uint64_t>& ids, std::uint64_t mask,
    std::vector<std::uint64_t>& scratch) -> bool {
    scratch.clear();

    for (auto id : ids) {
        scratch.push_back(id & mask);
    }

    std::sort(scratch.begin(), scratch.end());

    return std::adjacent_find(scratch.begin(), scratch.end()) == scratch.end();
}

template<std::size_t MaxBits>
template<class Registry>
auto minimal_cover_hash<MaxBits>::fn<Registry>::collisions(
    const std::vector<std::uint64_t>& ids, std::uint64_t mask,
    std::vector<std::uint64_t>& scratch) -> std::size_t {
    scratch.clear();

    for (auto id : ids) {
        scratch.push_back(id & mask);
    }

    std::sort(scratch.begin(), scratch.end());
    std::size_t count = 0;

    for (std::size_t i = 1; i < scratch.size(); ++i) {
        count += scratch[i] == scratch[i - 1];
    }

    return count;
}

template<std::size_t MaxBits>
template<class Registry>
auto minimal_cover_hash<MaxBits>::fn<Registry>::minimal_cover(
    const std::vector<std::uint64_t>& ids) -> std::uint64_t {
    // The bits that vary at all: trivially a cover, since two distinct ids
    // differ in at least one of them.
    std::uint64_t universe = 0;

    for (auto id : ids) {
        universe |= id ^ ids.front();
    }

    // Entropy of each varying bit: a bit almost every id agrees on separates
    // few pairs, so it is the first candidate for dropping.
    struct bit_info {
        int bit;
        double entropy;
    };

    std::vector<bit_info> bits;

    for (int bit = 0; bit < 64; ++bit) {
        if (!((universe >> bit) & 1)) {
            continue;
        }

        std::size_t ones = 0;

        for (auto id : ids) {
            ones += (id >> bit) & 1;
        }

        auto p = double(ones) / double(ids.size());
        auto entropy = (p <= 0.0 || p >= 1.0)
            ? 0.0
            : -(p * std::log2(p) + (1 - p) * std::log2(1 - p));
        bits.push_back({bit, entropy});
    }

    std::stable_sort(
        bits.begin(), bits.end(), [](const bit_info& a, const bit_info& b) {
            return a.entropy < b.entropy;
        });

    std::vector<std::uint64_t> scratch;
    scratch.reserve(ids.size());

    auto drop = [&](std::uint64_t mask) {
        for (const auto& info : bits) {
            auto candidate = mask & ~(std::uint64_t(1) << info.bit);

            if (candidate != mask && injective(ids, candidate, scratch)) {
                mask = candidate;
            }
        }

        return mask;
    };

    auto by_drop = drop(universe);

    // Bottom-up: add the bit that resolves the most collisions.
    std::uint64_t by_add = 0;

    while (!injective(ids, by_add, scratch)) {
        int best_bit = -1;
        std::size_t best_count = (std::numeric_limits<std::size_t>::max)();

        for (const auto& info : bits) {
            auto bit = std::uint64_t(1) << info.bit;

            if (by_add & bit) {
                continue;
            }

            auto count = collisions(ids, by_add | bit, scratch);

            if (count < best_count) {
                best_count = count;
                best_bit = info.bit;
            }
        }

        by_add |= std::uint64_t(1) << best_bit;
    }

    by_add = drop(by_add);

    return detail::popcount64(by_add) < detail::popcount64(by_drop) ? by_add
                                                                    : by_drop;
}

template<std::size_t MaxBits>
template<class Registry>
template<class Context, class... Options>
auto minimal_cover_hash<MaxBits>::fn<Registry>::initialize(
    const Context& ctx, const std::tuple<Options...>& options) -> void {
    (void)options;

    std::vector<std::uint64_t> ids;

    for (auto iter = ctx.classes_begin(); iter != ctx.classes_end(); ++iter) {
        for (auto type_iter = iter->type_id_begin();
             type_iter != iter->type_id_end(); ++type_iter) {
            ids.push_back(
                static_cast<std::uint64_t>(
                    reinterpret_cast<detail::uintptr>(*type_iter)));
        }
    }

    // One class may be registered under the same type id by several modules;
    // the cover is over *distinct* ids.
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());

    if (ids.empty()) {
        st().mask = 0;
        st().max_value = 0;

        return;
    }

    auto mask = ids.size() == 1 ? 0 : minimal_cover(ids);
    auto bits = detail::popcount64(mask);

    if (bits > MaxBits) {
        too_many_bits error;
        error.classes = ids.size();
        error.bits = bits;

        if constexpr (Registry::has_error_handler) {
            Registry::error_handler::error(error);
        }

        abort();
    }

    st().mask = mask;
    st().max_value = 0;

    for (auto id : ids) {
        st().max_value =
            (std::max)(st().max_value, std::size_t(detail::pext64(id, mask)));
    }

    if constexpr (Context::template has_option<trace>) {
        ctx.tr << "  type ids: " << ids.size() << ", cover: " << bits
               << " bits, table: " << (st().max_value + 1) << " slots\n";
    }

    if constexpr (Registry::has_runtime_checks) {
        st().control.assign(st().max_value + 1, type_id(detail::uintptr_max));

        for (auto id : ids) {
            st().control[std::size_t(detail::pext64(id, mask))] =
                reinterpret_cast<type_id>(id);
        }
    }
}

template<std::size_t MaxBits>
template<class Registry>
void minimal_cover_hash<MaxBits>::fn<Registry>::check(
    std::size_t index, type_id type) {
    if (index > st().max_value || st().control[index] != type) {
        if constexpr (Registry::has_error_handler) {
            missing_class error;
            error.type = type;
            Registry::error_handler::error(error);
        }

        abort();
    }
}

template<std::size_t MaxBits>
template<class Registry, class Stream>
auto minimal_cover_hash<MaxBits>::too_many_bits::write(Stream& os) const
    -> void {
    os << "the smallest bit cover of " << classes << " type ids is " << bits
       << " bits wide, more than the " << MaxBits << " allowed\n";
}

} // namespace policies
} // namespace boost::openmethod

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
