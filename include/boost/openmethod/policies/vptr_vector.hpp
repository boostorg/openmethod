// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICY_VPTR_VECTOR_HPP
#define BOOST_OPENMETHOD_POLICY_VPTR_VECTOR_HPP

#include <boost/openmethod/preamble.hpp>

#include <tuple>
#include <variant>
#include <vector>

namespace boost::openmethod {

namespace policies {

//! Stores v-table pointers in a vector.
//!
//! `vptr_vector` stores v-table pointers in a global vector. If `Registry`
//! contains a @ref type_hash policy, it is used to convert `type_id`s to
//! indices. Otherwise, `type_id`s are used directly as indices.
//!
//! If the registry contains the @ref indirect_vptr policy, stores pointers to
//! pointers to v-tables in the vector.
struct vptr_vector : vptr {
  public:
    //! A VptrFn metafunction.
    //!
    //! Keeps track of v-table pointers using a `std::vector`.
    //!
    //! If `Registry` contains a @ref type_hash policy, it is used to convert
    //! `type_id`s to indices; otherwise, `type_id`s are used as indices.
    //!
    //! If `Registry` contains the @ref indirect_vptr policy, stores pointers to
    //! pointers to v-tables in the map.
    //!
    //! @tparam Registry The registry containing this policy.
    template<class Registry>
    struct fn {
        using type_hash =
            typename Registry::template policy<policies::type_hash>;
        static constexpr auto has_type_hash = !std::is_same_v<type_hash, void>;

        // vptr_vector::initialize reads the type_hash policy's state, so that
        // policy must have been initialized first, i.e. must appear before
        // vptr_vector in the registry's policy_list (policies are initialized
        // left to right; see detail::initialize_policies). type_hash aliases
        // the policy's fn<Registry>, so locate the policy itself in the list.
        static_assert(
            !has_type_hash ||
                boost::mp11::mp_find<
                    typename Registry::policy_list,
                    detail::find_first_derived_of<
                        policies::type_hash, typename Registry::policy_list>>::
                        value <
                    boost::mp11::mp_find<
                        typename Registry::policy_list, vptr_vector>::value,
            "the type_hash policy must appear before vptr_vector in the "
            "registry's policy_list");

      public:
        //! The policy's state: the vector of v-table pointers. Held in the
        //! registry's shared state (see @ref registry_state).
        struct state {
            //! The v-table pointers (pointers to v-table pointers if
            //! `Registry` contains the @ref indirect_vptr policy), indexed by
            //! (possibly hashed) type ids.
            std::conditional_t<
                Registry::has_indirect_vptr,
                std::vector<const vptr_type*>,
                std::vector<vptr_type>> vptrs;
        };

        //! Stores the v-table pointers.
        //!
        //! If `Registry` contains a @ref type_hash policy, its `initialize`
        //! function is called. Its result determines the size of the vector.
        //! The v-table pointers are copied into the vector.
        //!
        //! @tparam Context An @ref InitializeContext.
        //! @tparam Options... Zero or more option types.
        //! @param ctx A Context object.
        //! @param options A tuple of option objects.
        template<class Context, class... Options>
        static auto
        initialize(const Context& ctx, const std::tuple<Options...>& options)
            -> void {
            std::size_t size;
            (void)options;

            if constexpr (has_type_hash) {
                auto [_, max_value] = type_hash::hash_range();
                size = max_value + 1;
            } else {
                size = 0;

                for (auto iter = ctx.classes_begin(); iter != ctx.classes_end();
                     ++iter) {
                    for (auto type_iter = iter->type_id_begin();
                         type_iter != iter->type_id_end(); ++type_iter) {
                        size = (std::max)(size, std::size_t(*type_iter));
                    }
                }

                ++size;
            }

            if constexpr (Registry::has_indirect_vptr) {
                st().vptrs.resize(size);
            } else {
                st().vptrs.resize(size);
            }

            for (auto iter = ctx.classes_begin(); iter != ctx.classes_end();
                 ++iter) {
                for (auto type_iter = iter->type_id_begin();
                     type_iter != iter->type_id_end(); ++type_iter) {
                    std::size_t index;

                    if constexpr (has_type_hash) {
                        index = type_hash::hash(*type_iter);
                    } else {
                        index = std::size_t(*type_iter);
                    }

                    if constexpr (Registry::has_indirect_vptr) {
                        st().vptrs[index] =
                            &iter->vptr();
                    } else {
                        st().vptrs[index] = iter->vptr();
                    }
                }
            }
        }

        //! Returns a *reference* to a v-table pointer for an object.
        //!
        //! Acquires the dynamic @ref type_id of `arg`, using the registry's
        //! @ref rtti policy.
        //!
        //! If the registry has a @ref type_hash policy, uses it to convert the
        //! type id to an index; otherwise, uses the type_id as the index.
        //!
        //! If the registry contains the @ref runtime_checks policy, verifies
        //! that the index falls within the limits of the vector. If it does
        //! not, and if the registry contains a @ref error_handler policy, calls
        //! its @ref error function with a @ref missing_class value, then
        //! terminates the program with @ref abort.
        //!
        //! @tparam Class A registered class.
        //! @param arg A reference to a const object of type `Class`.
        //! @return A reference to a the v-table pointer for `Class`.
        template<class Class>
        static auto dynamic_vptr(const Class& arg) -> const vptr_type& {
            auto dynamic_type = Registry::rtti::dynamic_type(arg);
            std::size_t index;
            if constexpr (has_type_hash) {
                index = type_hash::hash(dynamic_type);
            } else {
                index = std::size_t(dynamic_type);

                if constexpr (Registry::has_runtime_checks) {
                    std::size_t max_index = 0;

                    if constexpr (Registry::has_indirect_vptr) {
                        max_index = st().vptrs.size();
                    } else {
                        max_index = st().vptrs.size();
                    }

                    if (index >= max_index) {
                        if constexpr (Registry::has_error_handler) {
                            missing_class error;
                            error.type = dynamic_type;
                            Registry::error_handler::error(error);
                        }

                        abort();
                    }
                }
            }

            if constexpr (Registry::has_indirect_vptr) {
                return *st().vptrs[index];
            } else {
                return st().vptrs[index];
            }
        }

        //! Releases the memory allocated by `initialize`.
        //!
        //! @tparam Options... Zero or more option types, deduced from the function
        //! arguments.
        //! @param options Zero or more option objects.
        template<class... Options>
        static auto finalize(const std::tuple<Options...>&) -> void {
            using namespace policies;

            if constexpr (Registry::has_indirect_vptr) {
                st().vptrs.clear();
            } else {
                st().vptrs.clear();
            }
        }

      private:
        static auto& st() {
            return Registry::template state<vptr_vector>();
        }
    };
};

} // namespace policies
} // namespace boost::openmethod

#endif
