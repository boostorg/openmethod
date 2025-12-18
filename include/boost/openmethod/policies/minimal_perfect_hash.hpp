// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICY_MINIMAL_PERFECT_HASH_HPP
#define BOOST_OPENMETHOD_POLICY_MINIMAL_PERFECT_HASH_HPP

#include <boost/openmethod/preamble.hpp>

#include <limits>
#include <random>
#include <vector>
#include <algorithm>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702) // unreachable code
#endif

namespace boost::openmethod {

namespace detail {

#if defined(UINTPTR_MAX)
using uintptr = std::uintptr_t;
constexpr uintptr uintptr_max = UINTPTR_MAX;
#else
static_assert(
    sizeof(std::size_t) == sizeof(void*),
    "This implementation requires that size_t and void* have the same size.");
using uintptr = std::size_t;
constexpr uintptr uintptr_max = (std::numeric_limits<std::size_t>::max)();
#endif

template<class Registry>
std::vector<type_id> minimal_perfect_hash_control;

template<class Registry>
std::vector<std::size_t> minimal_perfect_hash_displacements;

} // namespace detail

namespace policies {

//! Hash type ids using a minimal perfect hash function.
//!
//! `minimal_perfect_hash` implements the @ref type_hash policy using a hash
//! function in the form `H(x)=(M*x)>>N`. It uses the PtHash algorithm to
//! determine values for `M` and `N` that result in a minimal perfect hash
//! function for the set of registered type_ids. This means that the hash
//! function is collision-free and the codomain is exactly the size of the
//! domain, resulting in a dense range [0, n-1] for n inputs.
//!
//! Unlike @ref fast_perfect_hash, which uses a hash table of size 2^k
//! (typically larger than needed) and may have unused slots, this policy
//! ensures the hash table has exactly n slots for n type_ids, with all
//! slots filled. This minimizes memory usage but may require more search
//! attempts during initialization.
struct minimal_perfect_hash : type_hash {

    //! Cannot find hash factors
    struct search_error : openmethod_error {
        //! Number of attempts to find hash factors
        std::size_t attempts;
        //! Number of buckets used in the last attempt
        std::size_t buckets;

        //! Write a short description to an output stream
        //! @param os The output stream
        //! @tparam Registry The registry
        //! @tparam Stream A @ref LightweightOutputStream
        template<class Registry, class Stream>
        auto write(Stream& os) const -> void;
    };

    using errors = std::variant<search_error>;

    //! A TypeHashFn metafunction.
    //!
    //! @tparam Registry The registry containing this policy
    template<class Registry>
    class fn {
        static std::size_t mult;
        static std::size_t shift;
        static std::size_t table_size;  // N for minimal perfect hash
        static std::size_t num_groups;
        static std::size_t group_mult;
        static std::size_t group_shift;

        static void check(std::size_t index, type_id type);

        template<class InitializeContext, class... Options>
        static void initialize(
            const InitializeContext& ctx, std::vector<type_id>& buckets,
            const std::tuple<Options...>& options);

      public:
        //! Find the hash factors using PtHash algorithm
        //!
        //! Uses the PtHash algorithm to find:
        //! - Pilot hash parameters (M, N) for H(x) = (M * x) >> N
        //! - Bucket assignment parameters 
        //! - Displacement values for each bucket to achieve minimal perfect hashing
        //!
        //! If no suitable values are found, calls the error handler with
        //! a @ref search_error object then calls `abort`.
        //!
        //! @tparam Context An @ref InitializeContext.
        //! @param ctx A Context object.
        //! @return A pair containing the minimum (0) and maximum (n-1) hash values.
        template<class Context, class... Options>
        static auto
        initialize(const Context& ctx, const std::tuple<Options...>& options) {
            if constexpr (Registry::has_runtime_checks) {
                initialize(
                    ctx, detail::minimal_perfect_hash_control<Registry>, options);
            } else {
                std::vector<type_id> buckets;
                initialize(ctx, buckets, options);
            }

            return std::pair{std::size_t(0), table_size - 1};
        }

        //! Hash a type id using the PtHash algorithm
        //!
        //! Hash a type id using H(x) = (pilot(x) + disp[group(x)]) % N
        //! where pilot(x) = (M * x) >> S and group(x) = (GM * x) >> GS.
        //!
        //! If `Registry` contains the @ref runtime_checks policy, checks that
        //! the type id is valid, i.e. if it was present in the set passed to
        //! @ref initialize. Its absence indicates that a class involved in a
        //! method definition, method overrider, or method call was not
        //! registered. In this case, signal a @ref missing_class using
        //! the registry's @ref error_handler if present; then calls `abort`.
        //!
        //! @param type The type_id to hash
        //! @return The hash value
        BOOST_FORCEINLINE
        static auto hash(type_id type) -> std::size_t {
            auto pilot = (mult * reinterpret_cast<detail::uintptr>(type)) >> shift;
            auto group = (group_mult * reinterpret_cast<detail::uintptr>(type)) >> group_shift;
            auto index = (pilot + detail::minimal_perfect_hash_displacements<Registry>[group]) % table_size;

            if constexpr (Registry::has_runtime_checks) {
                check(index, type);
            }

            return index;
        }

        //! Releases the memory allocated by `initialize`.
        //!
        //! @tparam Options... Zero or more option types, deduced from the function
        //! arguments.
        //! @param options Zero or more option objects.
        template<class... Options>
        static auto finalize(const std::tuple<Options...>&) -> void {
            detail::minimal_perfect_hash_control<Registry>.clear();
            detail::minimal_perfect_hash_displacements<Registry>.clear();
        }
    };
};

template<class Registry>
std::size_t minimal_perfect_hash::fn<Registry>::mult;

template<class Registry>
std::size_t minimal_perfect_hash::fn<Registry>::shift;

template<class Registry>
std::size_t minimal_perfect_hash::fn<Registry>::table_size;

template<class Registry>
std::size_t minimal_perfect_hash::fn<Registry>::num_groups;

template<class Registry>
std::size_t minimal_perfect_hash::fn<Registry>::group_mult;

template<class Registry>
std::size_t minimal_perfect_hash::fn<Registry>::group_shift;

template<class Registry>
template<class InitializeContext, class... Options>
void minimal_perfect_hash::fn<Registry>::initialize(
    const InitializeContext& ctx, std::vector<type_id>& buckets,
    const std::tuple<Options...>& options) {
    (void)options;

    const auto N = std::distance(ctx.classes_begin(), ctx.classes_end());

    if constexpr (InitializeContext::template has_option<trace>) {
        ctx.tr << "Finding minimal perfect hash using PtHash for " << N << " types\n";
    }

    // Table size is exactly N for minimal perfect hash
    table_size = N;
    
    if (table_size == 0) {
        shift = 0;
        mult = 1;
        num_groups = 0;
        group_mult = 1;
        group_shift = 0;
        detail::minimal_perfect_hash_displacements<Registry>.clear();
        return;
    }
    
    if (table_size == 1) {
        // Special case: only one type
        constexpr std::size_t bits_per_type_id = 8 * sizeof(type_id);
        shift = bits_per_type_id;
        mult = 1;
        num_groups = 1;
        group_mult = 1;
        group_shift = bits_per_type_id;
        detail::minimal_perfect_hash_displacements<Registry>.assign(1, 0);
        buckets.resize(1);
        for (auto iter = ctx.classes_begin(); iter != ctx.classes_end(); ++iter) {
            for (auto type_iter = iter->type_id_begin();
                 type_iter != iter->type_id_end(); ++type_iter) {
                buckets[0] = *type_iter;
            }
        }
        return;
    }

    // Collect all type_ids
    std::vector<type_id> keys;
    for (auto iter = ctx.classes_begin(); iter != ctx.classes_end(); ++iter) {
        for (auto type_iter = iter->type_id_begin();
             type_iter != iter->type_id_end(); ++type_iter) {
            keys.push_back(*type_iter);
        }
    }

    // Constants for PtHash algorithm
    constexpr std::size_t DEFAULT_RANDOM_SEED = 13081963; // Same seed as fast_perfect_hash
    constexpr std::size_t MAX_PASSES = 10;
    constexpr std::size_t MAX_ATTEMPTS = 100000;
    constexpr std::size_t DEFAULT_GROUP_DIVISOR = 4;  // N/4 groups for balance between memory and speed
    constexpr std::size_t DISTRIBUTION_FACTOR = 2;     // 2*N range for better distribution
    constexpr std::size_t bits_per_type_id = 8 * sizeof(type_id);

    std::default_random_engine rnd(DEFAULT_RANDOM_SEED);
    std::uniform_int_distribution<std::size_t> uniform_dist;
    std::size_t total_attempts = 0;

    // PtHash algorithm: partition keys into groups, then find displacements
    // Number of groups: typically sqrt(N) to N/4 for good performance
    num_groups = (std::max)(std::size_t(1), table_size / DEFAULT_GROUP_DIVISOR);
    if (num_groups > table_size) num_groups = table_size;
    
    // Calculate bits needed for num_groups
    std::size_t GM = 0;
    std::size_t power = 1;
    while (power < num_groups) {
        power <<= 1;
        ++GM;
    }
    group_shift = bits_per_type_id - GM;

    if constexpr (InitializeContext::template has_option<trace>) {
        ctx.tr << "  Using " << num_groups << " groups for " << table_size << " keys\n";
    }

    // Try different pilot hash parameters
    for (std::size_t pass = 0; pass < MAX_PASSES && total_attempts < MAX_ATTEMPTS; ++pass) {
        mult = uniform_dist(rnd) | 1;
        group_mult = uniform_dist(rnd) | 1;
        
        // Calculate M for pilot hash (number of bits for table_size range)
        std::size_t M = 0;
        power = 1;
        while (power < table_size * DISTRIBUTION_FACTOR) {
            power <<= 1;
            ++M;
        }
        shift = bits_per_type_id - M;

        // Partition keys into groups
        std::vector<std::vector<type_id>> groups(num_groups);
        for (auto key : keys) {
            auto group_idx = ((group_mult * reinterpret_cast<detail::uintptr>(key)) >> group_shift) % num_groups;
            groups[group_idx].push_back(key);
        }

        // Try to find displacements for each group
        detail::minimal_perfect_hash_displacements<Registry>.assign(num_groups, 0);
        buckets.assign(table_size, type_id(detail::uintptr_max));
        std::vector<bool> used(table_size, false);
        bool success = true;

        // Process groups in descending order of size (larger groups first)
        std::vector<std::size_t> group_order(num_groups);
        for (std::size_t i = 0; i < num_groups; ++i) group_order[i] = i;
        std::sort(group_order.begin(), group_order.end(), 
                  [&groups](std::size_t a, std::size_t b) {
                      return groups[a].size() > groups[b].size();
                  });

        for (auto g : group_order) {
            if (groups[g].empty()) continue;

            // Try different displacement values
            bool found = false;
            for (std::size_t disp = 0; disp < table_size * DISTRIBUTION_FACTOR && !found; ++disp) {
                ++total_attempts;
                if (total_attempts > MAX_ATTEMPTS) {
                    success = false;
                    break;
                }

                // Check if this displacement works for all keys in group
                std::vector<std::size_t> positions;
                positions.reserve(groups[g].size());
                bool valid = true;
                for (auto key : groups[g]) {
                    auto pilot = (mult * reinterpret_cast<detail::uintptr>(key)) >> shift;
                    auto pos = (pilot + disp) % table_size;
                    if (used[pos]) {
                        valid = false;
                        break;
                    }
                    positions.push_back(pos);
                }

                if (valid) {
                    // Mark positions as used and store keys
                    detail::minimal_perfect_hash_displacements<Registry>[g] = disp;
                    for (std::size_t i = 0; i < groups[g].size(); ++i) {
                        used[positions[i]] = true;
                        buckets[positions[i]] = groups[g][i];
                    }
                    found = true;
                }
            }

            if (!found) {
                success = false;
                break;
            }
        }

        if (success) {
            // Verify all positions are used (minimal property)
            bool all_used = true;
            for (std::size_t i = 0; i < table_size; ++i) {
                if (detail::uintptr(buckets[i]) == detail::uintptr_max) {
                    all_used = false;
                    break;
                }
            }

            if (all_used) {
                if constexpr (InitializeContext::template has_option<trace>) {
                    ctx.tr << "  Found minimal perfect hash after " << total_attempts
                           << " attempts\n";
                }
                return;
            }
        }
    }

    // Failed to find minimal perfect hash
    search_error error;
    error.attempts = total_attempts;
    error.buckets = table_size;

    if constexpr (Registry::has_error_handler) {
        Registry::error_handler::error(error);
    }

    abort();
}

template<class Registry>
void minimal_perfect_hash::fn<Registry>::check(std::size_t index, type_id type) {
    if (index >= table_size ||
        detail::minimal_perfect_hash_control<Registry>[index] != type) {

        if constexpr (Registry::has_error_handler) {
            missing_class error;
            error.type = type;
            Registry::error_handler::error(error);
        }

        abort();
    }
}

template<class Registry, class Stream>
auto minimal_perfect_hash::search_error::write(Stream& os) const -> void {
    os << "could not find minimal perfect hash factors after " << attempts 
       << " attempts using " << buckets << " buckets\n";
}

} // namespace policies
} // namespace boost::openmethod

#endif
