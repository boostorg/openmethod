// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_INTEROP_BOOST_TYPE_ERASURE_HPP
#define BOOST_OPENMETHOD_INTEROP_BOOST_TYPE_ERASURE_HPP

#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/call.hpp>
#include <boost/type_erasure/concept_interface.hpp>
#include <boost/type_erasure/concept_of.hpp>
#include <boost/type_erasure/derived.hpp>
#include <boost/type_erasure/placeholder.hpp>
#include <boost/type_erasure/typeid_of.hpp>

#include <boost/openmethod/core.hpp>
#include <boost/openmethod/interop/virtual_any.hpp>
#include <boost/openmethod/policies/std_rtti.hpp>

#include <type_traits>
#include <utility>

// Dispatch on the type contained in a boost::type_erasure::any.
//
// The Concept must contain boost::type_erasure::typeid_<> - which
// `relaxed` already implies - so that `typeid_of` can identify the
// contained value. Dispatch is on the `std::type_info` object returned by
// `typeid_of`: the type of the contained value for the owning any
// (`any<Concept>`), or the type *bound at construction* for the any
// references (`any<Concept, _self&>`, `any<Concept, const _self&>`) - never
// the C++ RTTI dynamic type of the referent.
//
// Supported virtual parameter forms:
// - `virtual_<const any<Concept>&>`, `virtual_<any<Concept>&>`,
//   `virtual_<any<Concept>&&>` - the owning any, by reference, like
//   `std::any`;
// - `virtual_<any<Concept, _self&>>` and
//   `virtual_<any<Concept, const _self&>>` - the any references, by
//   value (they are cheap, two-word handles);
// - `virtual_any<any<Concept>>` - looks the v-table pointer up once, at
//   construction. The Concept needs `relaxed` for virtual_any's default
//   constructor and assignment, and `copy_constructible<>` for copies.
//
// The rvalue-reference placeholder (`_self&&`), and placeholders other
// than `_self`, are not supported.
//
// The bound types are registered automatically, as classes derived from
// the owning any - the root class for the Concept: naming a type as an
// overrider parameter registers it; so does storing a value in a
// virtual_any.
//
// In addition, `openmethod_vptr` is a Boost.TypeErasure concept that
// stores the v-table pointer for the bound type in the any's own dispatch
// table, making every any on that Concept intrinsically polymorphic: calls
// resolve in constant time, without hashing the result of `typeid_of`,
// and binding a value to the any registers its type.
//
// This interop is based on a design contributed by Steven Watanabe:
// https://github.com/boostorg/openmethod/issues/21

namespace boost::openmethod {

namespace detail {

// Classification of the placeholder of an any<Concept, T>. `T = _self`
// (or any non-reference placeholder): the any owns the value. `T =
// _self&`: non-owning handle to a mutable referent. Anything else
// (`const _self&`, `_self&&`) is treated as binding a value that may not
// be mutated or moved from.
template<typename T>
constexpr bool te_owning = !std::is_reference_v<T>;

template<typename T>
constexpr bool te_mutable_bound = std::is_lvalue_reference_v<T> &&
    !std::is_const_v<std::remove_reference_t<T>>;

// Does U, an overrider parameter type, require mutable access?
template<typename U>
constexpr bool te_mutable_target = std::is_lvalue_reference_v<U> &&
    !std::is_const_v<std::remove_reference_t<U>>;

// Is U, an overrider parameter type, the `any` itself (by value or by
// any reference category)? Then the argument is passed through
// unchanged - the catch-all overrider case - instead of going through
// any_cast, which would throw unless the any contains an any.
template<typename U, class Any>
constexpr bool te_pass_through =
    std::is_same_v<std::remove_cv_t<std::remove_reference_t<U>>, Any>;

// The canonical root class for a Concept is the owning any. All the
// virtual_traits below use it as their virtual_type, whatever the
// placeholder of the parameter, so methods and overriders agree on a
// single registered root per Concept.

template<class C, typename T, class ParamRegistry, class Registry>
struct validate_method_parameter<
    virtual_<const boost::type_erasure::any<C, T>&, ParamRegistry>, Registry,
    void> : std::true_type {};

template<class C, typename T, class ParamRegistry, class Registry>
struct validate_method_parameter<
    virtual_<boost::type_erasure::any<C, T>&, ParamRegistry>, Registry, void> :
    std::true_type {};

template<class C, typename T, class ParamRegistry, class Registry>
struct validate_method_parameter<
    virtual_<boost::type_erasure::any<C, T>&&, ParamRegistry>, Registry, void> :
    std::true_type {};

template<class C, typename T, class ParamRegistry, class Registry>
struct validate_method_parameter<
    virtual_<boost::type_erasure::any<C, T&>, ParamRegistry>, Registry, void> :
    std::true_type {};

template<class C, typename T, class ParamRegistry, class Registry>
struct validate_method_parameter<
    virtual_<boost::type_erasure::any<C, const T&>, ParamRegistry>, Registry,
    void> : std::true_type {};

template<class C, class ParamRegistry, class Registry>
struct validate_method_parameter<
    virtual_<
        boost::type_erasure::any<C, boost::type_erasure::_self>, ParamRegistry>,
    Registry, void> : std::false_type {
    static_assert(
        false_t<C>, "an owning type_erasure::any must be passed by reference");
};

// `boost::type_erasure::typeid_of` yields a `std::type_info`, which is a valid
// `type_id` only for an rtti policy that identifies classes by `&typeid(T)`.
// Under any other policy the lookup key is meaningless, and `type_id` being
// `const void*`, nothing would diagnose it. The `openmethod_vptr` concept does
// not go through `typeid_of`, and is deliberately not covered.
template<class Registry>
constexpr void assert_std_rtti_type_erasure() {
    static_assert(
        std::is_base_of_v<
            policies::std_rtti,
            find_first_derived_of<
                policies::rtti, typename Registry::policy_list>>,
        "requires standard RTTI");
}

} // namespace detail

//! Specialize virtual_traits for `const boost::type_erasure::any&`.
//!
//! Dispatch is based on the type of the value bound to the `any`,
//! obtained via `boost::type_erasure::typeid_of`. `Concept` must contain
//! `boost::type_erasure::typeid_<>`; `relaxed` implies it.
//!
//! This specialization serves the owning `any` (`any<Concept>`) and,
//! through a const `any`, the any references.
//!
//! @tparam C The `any`'s Concept.
//! @tparam T The `any`'s placeholder.
//! @tparam Registry A @ref registry.
//!
//! @see [Interoperation with Boost.TypeErasure](xref:ROOT:interop_type_erasure.adoc)
template<class C, typename T, class Registry>
struct virtual_traits<const boost::type_erasure::any<C, T>&, Registry> {
    //! The type used for dispatch: the owning `any` for `C`.
    using virtual_type = boost::type_erasure::any<C>;

    //! Returns a const reference to the `any` argument.
    //! @param arg A reference to an `any`.
    //! @return A const reference to `arg`.
    static auto peek(const boost::type_erasure::any<C, T>& arg)
        -> const boost::type_erasure::any<C, T>& {
        return arg;
    }

    //! Returns a *reference* to a v-table pointer for the bound value.
    //!
    //! Looks up the @ref type_id returned by
    //! `boost::type_erasure::typeid_of` in the registry's `vptr` policy.
    //!
    //! This requires the registry's @ref rtti policy to be @ref std_rtti;
    //! the requirement is enforced with a `static_assert`. Registers the
    //! owning `any` - the root class of the bound types - in `Registry`.
    //!
    //! @param arg A reference to a const `any`.
    //! @return A reference to the v-table pointer for the bound value.
    static auto vptr(const boost::type_erasure::any<C, T>& arg)
        -> const vptr_type& {
        detail::assert_std_rtti_type_erasure<Registry>();
        (void)&detail::use_any_classes<Registry, boost::type_erasure::any<C>>;
        return Registry::vptr::vptr(&boost::type_erasure::typeid_of(arg));
    }

    //! Cast to a type.
    //!
    //! Extracts the bound value using `boost::type_erasure::any_cast`,
    //! and registers `U`, stripped of reference and cv-qualifiers, in
    //! `Registry` as a class derived from the owning `any`.
    //! Since the `any` is const, `U` can be a mutable reference only for
    //! the mutable any reference (`any<Concept, _self&>`), whose
    //! referent stays mutable through a const `any`. Rvalue references
    //! are never allowed; the overloads are removed from the overload
    //! set.
    //!
    //! @tparam U The target type (e.g. `const Dog&`, `Dog`).
    //! @param arg A reference to a const `any` method argument.
    //! @return The value bound to `arg`, cast to `U`.
    template<
        typename U,
        typename = std::enable_if_t<
            !std::is_rvalue_reference_v<U> &&
            (!detail::te_mutable_target<U> || detail::te_mutable_bound<T>)>>
    static auto cast(const boost::type_erasure::any<C, T>& arg)
        -> decltype(auto) {
        if constexpr (
            detail::te_pass_through<U, boost::type_erasure::any<C, T>>) {
            return (arg);
        } else {
            (void)&detail::use_any_classes<
                Registry, boost::type_erasure::any<C>, std::decay_t<U>>;
            return boost::type_erasure::any_cast<U>(arg);
        }
    }
};

//! Specialize virtual_traits for `boost::type_erasure::any&` (mutable
//! reference).
//!
//! Dispatch is based on the type of the value bound to the `any`,
//! obtained via `boost::type_erasure::typeid_of`. `Concept` must contain
//! `boost::type_erasure::typeid_<>`; `relaxed` implies it.
//!
//! @tparam C The `any`'s Concept.
//! @tparam T The `any`'s placeholder.
//! @tparam Registry A @ref registry.
//!
//! @see [Interoperation with Boost.TypeErasure](xref:ROOT:interop_type_erasure.adoc)
template<class C, typename T, class Registry>
struct virtual_traits<boost::type_erasure::any<C, T>&, Registry> {
    //! The type used for dispatch: the owning `any` for `C`.
    using virtual_type = boost::type_erasure::any<C>;

    //! Returns a const reference to the `any` argument.
    //! @param arg A reference to an `any`.
    //! @return A const reference to `arg`.
    static auto peek(const boost::type_erasure::any<C, T>& arg)
        -> const boost::type_erasure::any<C, T>& {
        return arg;
    }

    //! Returns a *reference* to a v-table pointer for the bound value.
    //!
    //! Looks up the @ref type_id returned by
    //! `boost::type_erasure::typeid_of` in the registry's `vptr` policy.
    //!
    //! This requires the registry's @ref rtti policy to be @ref std_rtti;
    //! the requirement is enforced with a `static_assert`. Registers the
    //! owning `any` - the root class of the bound types - in `Registry`.
    //!
    //! @param arg A reference to an `any`.
    //! @return A reference to the v-table pointer for the bound value.
    static auto vptr(const boost::type_erasure::any<C, T>& arg)
        -> const vptr_type& {
        detail::assert_std_rtti_type_erasure<Registry>();
        (void)&detail::use_any_classes<Registry, boost::type_erasure::any<C>>;
        return Registry::vptr::vptr(&boost::type_erasure::typeid_of(arg));
    }

    //! Cast to a type.
    //!
    //! Extracts the bound value using `boost::type_erasure::any_cast`,
    //! and registers `U`, stripped of reference and cv-qualifiers, in
    //! `Registry` as a class derived from the owning `any`.
    //! Supports mutable references (e.g. `Dog&`), except through the
    //! const any reference (`any<Concept, const _self&>`). `U` cannot
    //! be an rvalue reference: moving the value out must go through an
    //! explicit rvalue-reference parameter. The disallowed overloads are
    //! removed from the overload set.
    //!
    //! @tparam U The target type (e.g. `Dog&`, `const Dog&`, `Dog`).
    //! @param arg A mutable reference to the `any` method argument.
    //! @return The value bound to `arg`, cast to `U`.
    template<
        typename U,
        typename = std::enable_if_t<
            !std::is_rvalue_reference_v<U> &&
            (!detail::te_mutable_target<U> || detail::te_owning<T> ||
             detail::te_mutable_bound<T>)>>
    static auto cast(boost::type_erasure::any<C, T>& arg) -> decltype(auto) {
        if constexpr (
            detail::te_pass_through<U, boost::type_erasure::any<C, T>>) {
            return (arg);
        } else {
            (void)&detail::use_any_classes<
                Registry, boost::type_erasure::any<C>, std::decay_t<U>>;
            return boost::type_erasure::any_cast<U>(arg);
        }
    }
};

//! Specialize virtual_traits for `boost::type_erasure::any&&` (xvalue
//! reference).
//!
//! Dispatch is based on the type of the value bound to the `any`,
//! obtained via `boost::type_erasure::typeid_of`. `Concept` must contain
//! `boost::type_erasure::typeid_<>`; `relaxed` implies it.
//!
//! @tparam C The `any`'s Concept.
//! @tparam T The `any`'s placeholder.
//! @tparam Registry A @ref registry.
//!
//! @see [Interoperation with Boost.TypeErasure](xref:ROOT:interop_type_erasure.adoc)
template<class C, typename T, class Registry>
struct virtual_traits<boost::type_erasure::any<C, T>&&, Registry> {
    //! The type used for dispatch: the owning `any` for `C`.
    using virtual_type = boost::type_erasure::any<C>;

    //! Returns a const reference to the `any` argument.
    //! @param arg A reference to an `any`.
    //! @return A const reference to `arg`.
    static auto peek(const boost::type_erasure::any<C, T>& arg)
        -> const boost::type_erasure::any<C, T>& {
        return arg;
    }

    //! Returns a *reference* to a v-table pointer for the bound value.
    //!
    //! Looks up the @ref type_id returned by
    //! `boost::type_erasure::typeid_of` in the registry's `vptr` policy.
    //!
    //! This requires the registry's @ref rtti policy to be @ref std_rtti;
    //! the requirement is enforced with a `static_assert`. Registers the
    //! owning `any` - the root class of the bound types - in `Registry`.
    //!
    //! @param arg A reference to a const `any`.
    //! @return A reference to the v-table pointer for the bound value.
    static auto vptr(const boost::type_erasure::any<C, T>& arg)
        -> const vptr_type& {
        detail::assert_std_rtti_type_erasure<Registry>();
        (void)&detail::use_any_classes<Registry, boost::type_erasure::any<C>>;
        return Registry::vptr::vptr(&boost::type_erasure::typeid_of(arg));
    }

    //! Cast to a type.
    //!
    //! Extracts the bound value using `boost::type_erasure::any_cast`,
    //! and registers `U`, stripped of reference and cv-qualifiers, in
    //! `Registry` as a class derived from the owning `any`.
    //! `boost::type_erasure::any_cast` has no rvalue overload, so, for an
    //! rvalue-reference `U`, the result of a mutable-reference cast is
    //! moved - only for the owning `any`, since the rvalue-ness of an
    //! any reference says nothing about the referent. Casting to a
    //! value also moves for the owning `any`, and copies otherwise. The
    //! disallowed overloads are removed from the overload set.
    //!
    //! @tparam U The target type (e.g. `Dog&&`, `const Dog&`, `Dog`).
    //! @param arg An rvalue reference to the `any` method argument.
    //! @return The value bound to `arg`, cast to `U`.
    template<
        typename U,
        typename = std::enable_if_t<
            (!std::is_rvalue_reference_v<U> || detail::te_owning<T>) &&
            (!detail::te_mutable_target<U> || detail::te_owning<T> ||
             detail::te_mutable_bound<T>)>>
    static auto cast(boost::type_erasure::any<C, T>&& arg) -> decltype(auto) {
        if constexpr (
            detail::te_pass_through<U, boost::type_erasure::any<C, T>>) {
            return std::move(arg);
        } else {
            (void)&detail::use_any_classes<
                Registry, boost::type_erasure::any<C>, std::decay_t<U>>;

            if constexpr (std::is_rvalue_reference_v<U>) {
                return std::move(
                    boost::type_erasure::any_cast<std::remove_reference_t<U>&>(
                        arg));
            } else if constexpr (
                !std::is_reference_v<U> && detail::te_owning<T>) {
                return U(std::move(boost::type_erasure::any_cast<U&>(arg)));
            } else {
                return boost::type_erasure::any_cast<U>(arg);
            }
        }
    }
};

//! Specialize virtual_traits for the mutable any reference,
//! `boost::type_erasure::any<Concept, _self&>`, passed by value.
//!
//! The any references are cheap, two-word handles; passing them by
//! value is the idiomatic way to use them as parameters. Dispatch is on
//! the type *bound at construction*, obtained via
//! `boost::type_erasure::typeid_of` - not the C++ RTTI dynamic type of
//! the referent. `Concept` must contain `boost::type_erasure::typeid_<>`;
//! `relaxed` implies it.
//!
//! @tparam C The `any`'s Concept.
//! @tparam T The referent placeholder (`_self` for `any<C, _self&>`).
//! @tparam Registry A @ref registry.
//!
//! @see [Interoperation with Boost.TypeErasure](xref:ROOT:interop_type_erasure.adoc)
template<class C, typename T, class Registry>
struct virtual_traits<boost::type_erasure::any<C, T&>, Registry> {
    //! The type used for dispatch: the owning `any` for `C`.
    using virtual_type = boost::type_erasure::any<C>;

    //! Returns a const reference to the `any` argument.
    //! @param arg A reference to an `any`.
    //! @return A const reference to `arg`.
    static auto peek(const boost::type_erasure::any<C, T&>& arg)
        -> const boost::type_erasure::any<C, T&>& {
        return arg;
    }

    //! Returns a *reference* to a v-table pointer for the bound value.
    //!
    //! Looks up the @ref type_id returned by
    //! `boost::type_erasure::typeid_of` in the registry's `vptr` policy.
    //!
    //! This requires the registry's @ref rtti policy to be @ref std_rtti;
    //! the requirement is enforced with a `static_assert`. Registers the
    //! owning `any` - the root class of the bound types - in `Registry`.
    //!
    //! @param arg A reference to a const `any`.
    //! @return A reference to the v-table pointer for the bound value.
    static auto vptr(const boost::type_erasure::any<C, T&>& arg)
        -> const vptr_type& {
        detail::assert_std_rtti_type_erasure<Registry>();
        (void)&detail::use_any_classes<Registry, boost::type_erasure::any<C>>;
        return Registry::vptr::vptr(&boost::type_erasure::typeid_of(arg));
    }

    //! Cast to a type.
    //!
    //! Extracts the referent using `boost::type_erasure::any_cast`, and
    //! registers `U`, stripped of reference and cv-qualifiers, in
    //! `Registry` as a class derived from the owning `any`.
    //! Supports mutable references (e.g. `Dog&`); modifications through
    //! the result are visible through the referent. `U` cannot be an
    //! rvalue reference - the referent is not owned; the overloads are
    //! removed from the overload set.
    //!
    //! @tparam U The target type (e.g. `Dog&`, `const Dog&`, `Dog`).
    //! @param arg The any reference method argument.
    //! @return The value bound to `arg`, cast to `U`.
    template<
        typename U, typename = std::enable_if_t<!std::is_rvalue_reference_v<U>>>
    static auto cast(boost::type_erasure::any<C, T&> arg) -> decltype(auto) {
        if constexpr (
            detail::te_pass_through<U, boost::type_erasure::any<C, T&>>) {
            // by value: a reference would dangle when this function's
            // parameter goes out of scope
            return arg;
        } else {
            (void)&detail::use_any_classes<
                Registry, boost::type_erasure::any<C>, std::decay_t<U>>;
            return boost::type_erasure::any_cast<U>(arg);
        }
    }
};

//! Specialize virtual_traits for the const any reference,
//! `boost::type_erasure::any<Concept, const _self&>`, passed by value.
//!
//! The any references are cheap, two-word handles; passing them by
//! value is the idiomatic way to use them as parameters. Dispatch is on
//! the type *bound at construction*, obtained via
//! `boost::type_erasure::typeid_of` - not the C++ RTTI dynamic type of
//! the referent. `Concept` must contain `boost::type_erasure::typeid_<>`;
//! `relaxed` implies it.
//!
//! @tparam C The `any`'s Concept.
//! @tparam T The referent placeholder (`_self` for
//! `any<C, const _self&>`).
//! @tparam Registry A @ref registry.
//!
//! @see [Interoperation with Boost.TypeErasure](xref:ROOT:interop_type_erasure.adoc)
template<class C, typename T, class Registry>
struct virtual_traits<boost::type_erasure::any<C, const T&>, Registry> {
    //! The type used for dispatch: the owning `any` for `C`.
    using virtual_type = boost::type_erasure::any<C>;

    //! Returns a const reference to the `any` argument.
    //! @param arg A reference to an `any`.
    //! @return A const reference to `arg`.
    static auto peek(const boost::type_erasure::any<C, const T&>& arg)
        -> const boost::type_erasure::any<C, const T&>& {
        return arg;
    }

    //! Returns a *reference* to a v-table pointer for the bound value.
    //!
    //! Looks up the @ref type_id returned by
    //! `boost::type_erasure::typeid_of` in the registry's `vptr` policy.
    //!
    //! This requires the registry's @ref rtti policy to be @ref std_rtti;
    //! the requirement is enforced with a `static_assert`. Registers the
    //! owning `any` - the root class of the bound types - in `Registry`.
    //!
    //! @param arg A reference to a const `any`.
    //! @return A reference to the v-table pointer for the bound value.
    static auto vptr(const boost::type_erasure::any<C, const T&>& arg)
        -> const vptr_type& {
        detail::assert_std_rtti_type_erasure<Registry>();
        (void)&detail::use_any_classes<Registry, boost::type_erasure::any<C>>;
        return Registry::vptr::vptr(&boost::type_erasure::typeid_of(arg));
    }

    //! Cast to a type.
    //!
    //! Extracts the referent using `boost::type_erasure::any_cast`, and
    //! registers `U`, stripped of reference and cv-qualifiers, in
    //! `Registry` as a class derived from the owning `any`. Since
    //! the referent is const, `U` must be a value or a const reference;
    //! the other overloads are removed from the overload set.
    //!
    //! @tparam U The target type (e.g. `const Dog&`, `Dog`).
    //! @param arg The any reference method argument.
    //! @return The value bound to `arg`, cast to `U`.
    template<
        typename U,
        typename = std::enable_if_t<
            !std::is_rvalue_reference_v<U> && !detail::te_mutable_target<U>>>
    static auto cast(boost::type_erasure::any<C, const T&> arg)
        -> decltype(auto) {
        if constexpr (
            detail::te_pass_through<U, boost::type_erasure::any<C, const T&>>) {
            // by value: a reference would dangle when this function's
            // parameter goes out of scope
            return arg;
        } else {
            (void)&detail::use_any_classes<
                Registry, boost::type_erasure::any<C>, std::decay_t<U>>;
            return boost::type_erasure::any_cast<U>(arg);
        }
    }
};

//! A Boost.TypeErasure concept that makes an `any` intrinsically
//! polymorphic.
//!
//! Including `openmethod_vptr<Concept>` in a Concept adds an operation,
//! to the dispatch table of every `any` on `Concept`, that returns
//! the @ref registry::static_vptr for the bound type; and it surfaces the
//! operation as a @ref boost_openmethod_vptr overload, which dispatch
//! prefers over the registry's `vptr` policy. Calls thus resolve in
//! constant time, without hashing the result of
//! `boost::type_erasure::typeid_of`.
//!
//! In addition, binding a value to such an `any` registers its type as a
//! class derived from the owning `any` - the same shape that naming the
//! type as an overrider parameter produces, with which it can coexist.
//!
//! `Concept` must be the very Concept the `any` is instantiated with.
//! Since the concept appears inside that Concept, the Concept must name
//! itself: define it as a struct deriving from the concept list.
//!
//! Unlike the `vptr` policy, which reports a @ref missing_class error,
//! calling a method on an empty relaxed `any` throws
//! `boost::type_erasure::bad_function_call`.
//!
//! An `any` that carries this concept cannot be wrapped in a
//! @ref virtual_any. The hook returns the v-table pointer by value, and an
//! indirect registry cannot store that. Wrapping one is rejected at
//! compile time.
//!
//! Both give constant-time access to the v-table pointer, but not at the
//! same cost. This concept reaches it through an indirect call on the
//! `any`'s own dispatch table; a `virtual_any` loads it from the wrapper.
//! What the concept saves over a plain `any` is the hash of
//! `boost::type_erasure::typeid_of`, not the call.
//!
//! @tparam Concept The Concept containing this concept.
//! @tparam Registry A @ref registry.
//! @tparam T A placeholder; leave it to its default, `_self`.
//!
//! @par Example
//! include:type_erasure_concept.cpp#concept
//!
//! @see [Interoperation with Boost.TypeErasure](xref:ROOT:interop_type_erasure.adoc)
template<
    class Concept, class Registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY,
    typename T = boost::type_erasure::_self>
struct openmethod_vptr {
    //! Returns the v-table pointer for the bound type.
    //!
    //! Also registers `T`, and the owning `any` for `Concept` as its
    //! base, by odr-using their registrars.
    //!
    //! @return The @ref registry::static_vptr for `T`.
    static auto apply(const T&) -> vptr_type {
        (void)&detail::use_any_classes<
            Registry, boost::type_erasure::any<Concept>, T>;
        return Registry::template static_vptr<T>;
    }
};

} // namespace boost::openmethod

namespace boost::type_erasure {

// Surface the openmethod_vptr operation as the boost_openmethod_vptr
// intrinsic hook, injected into the interface of every any whose
// Concept contains the concept.
template<class Concept, class Registry, typename T, class Base>
struct concept_interface<
    boost::openmethod::openmethod_vptr<Concept, Registry, T>, Base, T> : Base {
    // The parameter is a deduced `Self`, constrained to the exact any
    // type, because MSVC's `/std:c++17` does not imply `/permissive-`:
    // it injects hidden friends into the enclosing namespace, where
    // ordinary lookup finds them. A `const derived<Base>::type&`
    // parameter would then make this a candidate for an any over an
    // unrelated Concept, which MSVC tries to convert to this one - and
    // the conversion fails outside the immediate context, so it is an
    // error, not a substitution failure.
    template<class Self>
    friend auto boost_openmethod_vptr(const Self& arg, Registry*)
        -> std::enable_if_t<
            std::is_same_v<Self, typename derived<Base>::type>,
            boost::openmethod::vptr_type> {
        return call(
            boost::openmethod::openmethod_vptr<Concept, Registry, T>(), arg);
    }
};

} // namespace boost::type_erasure

namespace boost::openmethod {

// The primary final_virtual_ptr would silently use the static v-table
// pointer of the any class itself - the root -, not the bound value's.
// Delete the combination. Both call forms need covering: the (C, T)-only
// templates catch calls that deduce the default registry, and the
// Registry-first templates catch explicit-registry calls; both are more
// specialized than the primary's forwarding-reference parameter.
//
// Hidden from the reference: they are a guard, not API, and six deleted
// overloads would crowd the `final_virtual_ptr` overload list.

#ifndef __MRDOCS__
template<class C, typename T>
void final_virtual_ptr(const boost::type_erasure::any<C, T>&) = delete;
template<class C, typename T>
void final_virtual_ptr(boost::type_erasure::any<C, T>&) = delete;
template<class C, typename T>
void final_virtual_ptr(boost::type_erasure::any<C, T>&&) = delete;
template<class Registry, class C, typename T>
void final_virtual_ptr(const boost::type_erasure::any<C, T>&) = delete;
template<class Registry, class C, typename T>
void final_virtual_ptr(boost::type_erasure::any<C, T>&) = delete;
template<class Registry, class C, typename T>
void final_virtual_ptr(boost::type_erasure::any<C, T>&&) = delete;
#endif

namespace aliases {
using boost::openmethod::openmethod_vptr;
} // namespace aliases

} // namespace boost::openmethod

#endif
