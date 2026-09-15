// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICY_VPTR_MAP_HPP
#define BOOST_OPENMETHOD_POLICY_VPTR_MAP_HPP

#include <boost/openmethod/preamble.hpp>

#include <tuple>
#include <unordered_map>

#ifdef _MSC_VER
#pragma warning(push)
// 4702: unreachable code. The `abort()` after a call to the error handler is
// there for a handler that returns - the default one prints and returns - but a
// handler that is [[noreturn]], like throw_error_handler, makes it dead code,
// and MSVC diagnoses that. Same reason as in preamble.hpp and core.hpp.
#pragma warning(disable : 4702)
#endif

namespace boost::openmethod {

namespace policies {

//! Stores v-table pointers in a map keyed by `type_id`s.
//!
//! `vptr_map` stores v-table pointers in a map keyed by `type_id`s.
//!
//! If the registry contains the @ref indirect_vptr policy, `vptr_map` stores
//! pointers to pointers to v-tables.
//!
//! @tparam MapFn A mp11 quoted metafunction that takes a key type and a
//! value type, and returns an
//! [AssociativeContainer](https://en.cppreference.com/w/cpp/named_req/AssociativeContainer).
//!
//! @par Example
//! include:policies.cpp#vptr_map
//!
//! @see [Registries and Policies](xref:ROOT:registries_and_policies.adoc)
template<class MapFn = mp11::mp_quote<std::unordered_map>>
class vptr_map : public vptr {
  public:
    //! A VptrFn metafunction.
    //!
    //! @tparam Registry The registry containing this policy.
    template<class Registry>
    class fn {
        using Value = std::conditional_t<
            Registry::has_indirect_vptr, const vptr_type*, vptr_type>;

      public:
        //! The policy's state: the map of v-table pointers. Held in the
        //! registry's shared state (see @ref registry_state).
        struct state {
            //! The v-table pointers (pointers to v-table pointers if
            //! `Registry` contains the @ref indirect_vptr policy), keyed by
            //! type id.
            typename MapFn::template fn<type_id, Value> vptrs;
        };

      private:
        static auto& st() {
            return Registry::template state<vptr_map<MapFn>>();
        }

      public:
        //! Stores the v-table pointers.
        //!
        //! @tparam Context An @ref InitializeContext.
        //! @tparam Options... Zero or more option types.
        //! @param ctx A Context object.
        //! @param options A tuple of option objects.
        template<class Context, class... Options>
        static void initialize(
            const Context& ctx, const std::tuple<Options...>&) {
            decltype(st().vptrs) new_vptrs;

            for (auto iter = ctx.classes_begin(); iter != ctx.classes_end();
                 ++iter) {
                for (auto type_iter = iter->type_id_begin();
                     type_iter != iter->type_id_end(); ++type_iter) {

                    if constexpr (Registry::has_indirect_vptr) {
                        new_vptrs.emplace(*type_iter, iter->static_vptr());
                    } else {
                        new_vptrs.emplace(*type_iter, iter->vptr());
                    }
                }
            }

            st().vptrs.swap(new_vptrs);
        }

        //! Returns a *reference* to a v-table pointer for an object.
        //!
        //! Acquires the dynamic @ref type_id of `arg`, using the registry's
        //! @ref rtti policy.
        //!
        //! If the registry contains the @ref runtime_checks policy, checks that
        //! the map contains the type id. If it does not, and if the registry
        //! contains a @ref error_handler policy, calls its
        //! @ref error function with a @ref missing_class value, then
        //! terminates the program with `abort`.
        //!
        //! @tparam Class A registered class.
        //! @param arg A reference to a const object of type `Class`.
        //! @return A reference to the v-table pointer for `Class`.
        template<class Class>
        static auto dynamic_vptr(const Class& arg) -> const vptr_type& {
            return vptr(Registry::rtti::dynamic_type(arg));
        }

        //! Returns a *reference* to a v-table pointer for a type.
        //!
        //! If the registry contains the @ref runtime_checks policy, checks that
        //! the map contains the type id. If it does not, and if the registry
        //! contains a @ref error_handler policy, calls its
        //! @ref error function with a @ref missing_class value, then
        //! terminates the program with `abort`.
        //!
        //! @param type A `type_id`.
        //! @return A reference to the v-table pointer for `type`.
        static auto vptr(type_id type) -> const vptr_type& {
            auto iter = st().vptrs.find(type);

            if constexpr (Registry::has_runtime_checks) {
                if (iter == st().vptrs.end()) {
                    if constexpr (Registry::has_error_handler) {
                        missing_class error;
                        error.type = type;
                        Registry::error_handler::error(error);
                    }

                    abort();
                }
            }

            if constexpr (Registry::has_indirect_vptr) {
                // check for valid iterator is done if runtime_checks is enabled
                // coverity[deref_iterator:SUPPRESS]
                return *iter->second;
            } else {
                // coverity[deref_iterator:SUPPRESS]
                return iter->second;
            }
        }

        //! Clears the map.
        //!
        //! @tparam Options... Zero or more option types.
        //! @param ctx A Context object.
        //! @param options A tuple of option objects.
        template<class... Options>
        static auto finalize(const std::tuple<Options...>&) -> void {
            st().vptrs.clear();
        }
    };
};

} // namespace policies
} // namespace boost::openmethod

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
