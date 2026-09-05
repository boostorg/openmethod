// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_CORE_HPP
#define BOOST_OPENMETHOD_CORE_HPP

#include <stdint.h>
#include <algorithm>
#include <cstdlib>
#include <tuple>
#include <type_traits>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/bind.hpp>
#include <boost/mp11/integral.hpp>
#include <boost/mp11/list.hpp>

#include <boost/openmethod/preamble.hpp>
#include <boost/openmethod/default_registry.hpp>
#include <boost/openmethod/detail/reflection.hpp>

#ifndef BOOST_OPENMETHOD_DEFAULT_REGISTRY
//! Default value for `Registry`.
//!
//! The name of the default registry.
//!
//! `BOOST_OPENMETHOD_DEFAULT_REGISTRY` is the default value for the `Registry`
//! template parameter of @ref boost::openmethod::method,
//! @ref boost::openmethod::use_classes, @ref boost::openmethod::virtual_ptr,
//! and all the constructs that take a registry as a template argument.
//!
//! `BOOST_OPENMETHOD_DEFAULT_REGISTRY` can be defined by a program to change
//! the default registry globally, *before* including
//! `<boost/openmethod/core.hpp>` (or any header that includes it, like
//! `<boost/openmethod.hpp>`). After that, changing its value has no effect,
//! even on other macros.
//!
//! To use a registry that the library provides, name it in the macro:
//!
//! @code
//! #define BOOST_OPENMETHOD_DEFAULT_REGISTRY boost::openmethod::indirect_registry
//! #include <boost/openmethod.hpp>
//! @endcode
//!
//! To use a registry of your own, *declare* the class before the include, and
//! *define* it after:
//!
//! @code
//! struct my_registry;
//! #define BOOST_OPENMETHOD_DEFAULT_REGISTRY my_registry
//!
//! #include <boost/openmethod.hpp>
//! // plus any policies/ and interop/ headers needed, in any order
//!
//! struct my_registry
//!     : boost::openmethod::default_registry::with<my_policy> {};
//! @endcode
//!
//! Only the declaration has to precede the include. Deferring the definition is
//! what makes it possible to build the registry from the policies that
//! `<boost/openmethod.hpp>` brings in - and from policies of your own, written
//! against them.
//!
//! The registry must be complete before the first construct that instantiates
//! it: a `BOOST_OPENMETHOD*` macro, @ref boost::openmethod::use_classes,
//! @ref boost::openmethod::method, @ref boost::openmethod::virtual_ptr, or
//! @ref BOOST_OPENMETHOD_IMPORT_REGISTRY and its companions.
//!
//! @note The value must name a class, not a typedef or an alias template, and
//! the declaration must use the same class-key as the definition. Qualify the
//! name (`::my_registry`, `myapp::my_registry`) if it could also be found in
//! namespace `boost::openmethod` - `registry` in particular.
//!
//! @note Use this feature with caution, as it will cause ODR violations if
//! different translation units define different default registries.
//!
//! @see [Registries and Policies](xref:ROOT:registries_and_policies.adoc)
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY ::boost::openmethod::default_registry
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4100)
#pragma warning(disable : 4646)
#pragma warning(disable : 4702) // unreachable code
#endif

//! Top namespace of the library.
namespace boost::openmethod {

// Hide the `detail::` qualification of the exposition-only traits from MrDocs,
// which documents them as members of `boost::openmethod`.
//
// MrDocs renders the condition of an `enable_if_t` used as a defaulted template
// argument as a C++20 requires-clause, by copying the *raw source text* spanning
// the condition expression - it does not walk the expression tree
// (cppalliance/mrdocs#1016). A macro is thus elided only when it sits *before*
// the first token of the condition, where it expands to nothing under
// `__MRDOCS__` and so falls outside the copied range; anywhere inside the
// condition - after a `!`, after a `&&`, inside parentheses - its name is
// printed verbatim. That holds whatever shape the macro has - the function-like
// form below, an object-like one, or a bare `#ifndef __MRDOCS__` around
// `detail::`.
//
// Hence the rule for a condition mentioning an exposition-only trait:
//
// - `BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)` must be the first thing in the
//   condition. Spell a negation as `Trait<T> == false`, not `!Trait<T>`.
// - One trait per condition. When a constraint needs several, give each its own
//   defaulted template parameter - MrDocs joins them with `&&`, in order, so the
//   rendered clause is unchanged, and substitution short-circuits at the first
//   failure.
//
// Only expressions are affected. Types are printed from the AST, so the macro
// may appear anywhere in one (see `method::operator()`).
#ifdef __MRDOCS__
#define BOOST_OPENMETHOD_OPEN_NAMESPACE_DETAIL_UNLESS_MRDOCS
#define BOOST_OPENMETHOD_CLOSE_NAMESPACE_DETAIL_UNLESS_MRDOCS
#define BOOST_OPENMETHOD_UNLESS_MRDOCS(...)
#else
#define BOOST_OPENMETHOD_OPEN_NAMESPACE_DETAIL_UNLESS_MRDOCS namespace detail {
#define BOOST_OPENMETHOD_CLOSE_NAMESPACE_DETAIL_UNLESS_MRDOCS }
#define BOOST_OPENMETHOD_UNLESS_MRDOCS(...) __VA_ARGS__
#endif

namespace detail {
using sfinae = void;
}

template<
    class Class, class Registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY,
    typename = detail::sfinae>
class virtual_ptr;

// =============================================================================
// Helpers

namespace detail {

using macro_default_registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY;

template<typename...>
struct extract_registry;

template<>
struct extract_registry<> {
    using registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY;
    using others = mp11::mp_list<>;
};

template<typename Type>
struct extract_registry<Type> {
    using registry = std::conditional_t<
        is_registry<Type>, Type, BOOST_OPENMETHOD_DEFAULT_REGISTRY>;
    using others = std::conditional_t<
        is_registry<Type>, mp11::mp_list<>, mp11::mp_list<Type>>;
};

template<typename Type1, typename Type2, typename... MoreTypes>
struct extract_registry<Type1, Type2, MoreTypes...> {
    static_assert(!is_registry<Type1>, "policy must be the last in the list");
    using registry = typename extract_registry<Type2, MoreTypes...>::registry;
    using others = mp11::mp_push_front<
        typename extract_registry<Type2, MoreTypes...>::others, Type1>;
};

template<class Registry, class... Class>
struct init_type_ids;

template<class Registry, class... Class>
struct init_type_ids<Registry, mp11::mp_list<Class...>> {
    static auto fn(type_id* ids) {
        (..., (*ids++ = Registry::rtti::template static_type<Class>()));
        return ids;
    }
};

template<class Base, class Derived>
struct is_unambiguous_accessible_base_of : std::is_base_of<Base, Derived> {
    static_assert(
        std::is_base_of_v<Base, Derived> ==
            std::is_convertible_v<Derived&, Base&>,
        "class must be an accessible unambiguous base, repeated inheritance is "
        "not "
        "supported");
};

// Collect the base classes of a list of classes. The result is a mp11 map that
// associates each class to a list starting with the class itself, followed by
// all its bases, as per std::is_base_of. Thus the list includes the class
// itself at least twice: at the front, and down the list, as its own improper
// base. The direct and indirect bases are all included. The runtime will
// extract the direct proper bases.
template<typename... Cs>
using inheritance_map = mp11::mp_list<boost::mp11::mp_push_front<
    boost::mp11::mp_filter_q<
        boost::mp11::mp_bind_back<is_unambiguous_accessible_base_of, Cs>,
        mp11::mp_list<Cs...>>,
    Cs>...>;

// =============================================================================
// optimal_cast

template<typename B, typename D, typename = void>
struct requires_dynamic_cast_ref_aux : std::true_type {};

template<typename B, typename D>
struct requires_dynamic_cast_ref_aux<
    B, D, std::void_t<decltype(static_cast<D>(std::declval<B>()))>> :
    std::false_type {};

template<class B, class D>
constexpr bool requires_dynamic_cast =
    detail::requires_dynamic_cast_ref_aux<B, D>::value;

template<class Registry, class D, class B>
auto optimal_cast(B&& obj) -> decltype(auto) {
    if constexpr (requires_dynamic_cast<B, D>) {
        return Registry::rtti::template dynamic_cast_ref<D>(
            std::forward<B>(obj));
    } else {
        return static_cast<D>(obj);
    }
}

// =============================================================================
// Common details

template<typename T>
struct is_virtual : std::false_type {};

template<typename T>
struct is_virtual<virtual_<T>> : std::true_type {};

template<typename T>
struct remove_virtual_aux {
    using type = T;
};

template<typename T>
struct remove_virtual_aux<virtual_<T>> {
    using type = T;
};

template<typename T>
using remove_virtual_ = typename remove_virtual_aux<T>::type;

template<typename T, class Registry, typename = void>
struct virtual_type_aux {
    using type = void;
};

template<typename T, class Registry>
struct virtual_type_aux<
    T, Registry,
    std::void_t<typename virtual_traits<T, Registry>::virtual_type>> {
    using type = typename virtual_traits<T, Registry>::virtual_type;
};

template<typename T, class Registry>
using virtual_type = typename virtual_type_aux<T, Registry>::type;

template<typename MethodArgList>
using virtual_types = boost::mp11::mp_transform<
    remove_virtual_, boost::mp11::mp_filter<detail::is_virtual, MethodArgList>>;

} // namespace detail

BOOST_OPENMETHOD_OPEN_NAMESPACE_DETAIL_UNLESS_MRDOCS

//! Removes the virtual_<> decorator, if present (exposition only).
//!
//! Provides a nested `type` equal to `T`. The template is specialized for
//! `virtual_<T>`.
//!
//! @tparam T A type.
template<typename T>
struct StripVirtualDecorator {
    //! Same as `T`
    using type = T;
};

//! Removes the virtual_<> decorator (exposition only).
//!
//! Provides a nested `type` equal to `T`.
//!
//! @tparam T A type.
template<typename T>
struct StripVirtualDecorator<virtual_<T>> {
    //! Same as `T`.
    using type = T;
};

BOOST_OPENMETHOD_CLOSE_NAMESPACE_DETAIL_UNLESS_MRDOCS

// =============================================================================
// virtual_traits

//! Traits for types used as virtual parameters.
//!
//! `virtual_traits` must be specialized for each type that can be used as a
//! virtual parameters. It enables methods to:
//! @li find the type of the object the argument refers to (e.g. @c Node from
//! @c Node&)
//! @li obtain a non-modifiable reference to that object (e.g. a @c const
//! @c Node& from @c Node&)
//! @li cast the argument to another type (e.g. cast a @c Node& to a @c Plus&)
//!
//! @par Requirements
//!
//! Specializations of `virtual_traits` must provide the members described to
//! the @ref VirtualTraits blueprint.
//!
//! @tparam T A type referring (in the broad sense) to an instance of a class.
//! @tparam Registry A @ref registry.
template<typename T, class Registry>
struct virtual_traits;

//! Specialize virtual_traits for lvalue reference types.
//!
//! @tparam Class A class type, possibly cv-qualified.
//! @tparam Registry A @ref registry.
template<class Class, class Registry>
struct virtual_traits<Class&, Registry> {
    //! `Class`, stripped from cv-qualifiers.
    using virtual_type = std::remove_cv_t<Class>;

    //! Return a reference to a non-modifiable `Class` object.
    //! @param arg A reference to a non-modifiable `Class` object.
    //! @return A reference to the same object.
    static auto peek(const Class& arg) -> const Class& {
        return arg;
    }

    //! Cast to another type.
    //!
    //! Cast an object to another type. If possible, use `static_cast`.
    //! Otherwise, use `Registry::rtti::dynamic_cast_ref`.
    //!
    //! @tparam Derived A lvalue reference type.
    //! @param obj A reference to a `Class` object.
    //! @return A reference to the same object, cast to `Derived`.
    template<typename Derived>
    static auto cast(Class& obj) -> Derived {
        static_assert(std::is_lvalue_reference_v<Derived>);
        return detail::optimal_cast<Registry, Derived>(obj);
    }
};

//! Specialize virtual_traits for xvalue reference types.
//!
//! @tparam T A xvalue reference type.
//! @tparam Registry A @ref registry.
template<class Class, class Registry>
struct virtual_traits<Class&&, Registry> {
    //! Same as `Class`.
    using virtual_type = Class;

    //! Return a reference to a non-modifiable `Class` object.
    //! @param arg A reference to a non-modifiable `Class` object.
    //! @return A reference to the same object.
    static auto peek(const Class& arg) -> const Class& {
        return arg;
    }

    //! Cast to another type.
    //!
    //! Cast an object to another type. If possible, use `static_cast`.
    //! Otherwise, use `Registry::rtti::dynamic_cast_ref`.
    //!
    //! @tparam Derived A rvalue reference type.
    //! @param obj A reference to a `Class` object.
    //! @return A reference to the same object, cast to `Derived`.
    template<typename Derived>
    static auto cast(Class&& obj) -> Derived {
        static_assert(std::is_rvalue_reference_v<Derived>);
        return detail::optimal_cast<Registry, Derived>(obj);
    }
};

//! Specialize virtual_traits for pointer types.
//!
//! @tparam Class A class type, possibly cv-qualified.
//! @tparam Registry A @ref registry.
template<class Class, class Registry>
struct virtual_traits<Class*, Registry> {
    //! `Class`, stripped from cv-qualifiers.
    using virtual_type = std::remove_cv_t<Class>;

    //! Return a reference to a non-modifiable `Class` object.
    //! @param arg A pointer to a non-modifiable `Class` object.
    //! @return A const reference to the same object.
    static auto peek(const Class* arg) -> const Class& {
        return *arg;
    }

    //! Cast to another type.
    //!
    //! Cast an object to another type. If possible, use `static_cast`.
    //! Otherwise, use `Registry::rtti::dynamic_cast_ref`.
    //!
    //! @tparam Derived A pointer type.
    //! @param obj A pointer to a `Class` object.
    //! @return A pointer to the same object, cast to `Derived`.
    template<typename Derived>
    static auto cast(Class* ptr) -> Derived {
        static_assert(std::is_pointer_v<Derived>);

        if constexpr (detail::requires_dynamic_cast<Class*, Derived>) {
            return dynamic_cast<Derived>(ptr);
        } else {
            return static_cast<Derived>(ptr);
        }
    }
};

namespace detail {

template<class...>
struct use_class_aux;

template<class Registry, class Class, typename... Bases>
struct use_class_aux<Registry, mp11::mp_list<Class, Bases...>> :
    std::conditional_t<
        Registry::has_deferred_static_rtti, detail::deferred_class_info,
        detail::class_info> {
    static type_id bases[sizeof...(Bases)];
    use_class_aux() {
        this->first_base = bases;
        this->last_base = bases + sizeof...(Bases);
        this->is_abstract = std::is_abstract_v<Class>;
        this->static_vptr = &Registry::template static_vptr<Class>;

        if constexpr (!Registry::has_deferred_static_rtti) {
            resolve_type_ids();
        }

        // coverity[uninit] - zero-initialized static storage
        Registry::static_::st.classes.push_back(*this);
    }

    void resolve_type_ids() {
        this->type = Registry::rtti::template static_type<Class>();
        auto iter = bases;
        (..., (*iter++ = Registry::rtti::template static_type<Bases>()));
    }

    ~use_class_aux() {
        Registry::static_::st.classes.remove(*this);
    }
};

template<class Registry, class Class, typename... Bases>
type_id use_class_aux<
    Registry, mp11::mp_list<Class, Bases...>>::bases[sizeof...(Bases)];

template<class... Classes>
using use_classes_tuple_type = boost::mp11::mp_apply<
    detail::tuple,
    boost::mp11::mp_transform_q<
        boost::mp11::mp_bind_front<
            detail::use_class_aux,
            typename detail::extract_registry<Classes...>::registry>,
        boost::mp11::mp_apply<
            detail::inheritance_map,
            boost::mp11::mp_unique<
                typename detail::extract_registry<Classes...>::others>>>>;

} // namespace detail

//! Add classes to a registry
//!
//! `use_classes` is a registrar class that adds one or more classes to a
//! registry.
//!
//! Classes potentially involved in a method definition, an overrider, or a
//! method call must be registered via `use_classes`. A class may be registered
//! multiple times. A class and its direct bases must be listed together in one
//! or more instantiations of `use_classes`.
//!
//! If a class is identified by different type ids in different translation
//! units, it must be registered in as many translation units as necessary for
//! `use_classes` to register all the type ids. This situation can occur when
//! using standard RTTI, because the address of the `type_info` objects are used
//! as type ids, and the standard does not guarantee that there is exactly one
//! such object per class. The only such case known to the author is when using
//! Windows DLLs.
//!
//! Virtual and multiple inheritance are supported, with the exclusion of
//! repeated inheritance.
//!
//! @see [Core API](xref:ROOT:core_api.adoc)
//! @see [Registries and Policies](xref:ROOT:registries_and_policies.adoc)
template<class... Classes>
class use_classes {
    detail::use_classes_tuple_type<Classes...> tuple;
};

// -----------------------------------------------------------------------------
// reflection-based class registration

namespace detail {

#if BOOST_OPENMETHOD_HAS_REFLECTION

// One registrar per entry, for the whole program - not per translation unit, as
// `BOOST_OPENMETHOD_CLASSES` produces. Same mechanism as
// `inplace_vptr_use_classes`: an `inline` variable template, instantiated by
// odr-use. Keyed on the entry rather than on the class, because a class' base
// list depends on what else was registered alongside it.
template<class Registry, class Entry>
inline use_class_aux<Registry, Entry> reflected_class_registrar;

// Register every class the scan selected for `Registry`, each with its
// registered ancestors, as `reflected_registered_classes` computed them.
template<class Registry, class... Entries>
BOOST_FORCEINLINE auto use_reflected_classes(
    mp11::mp_list<Registry, Entries...>*) -> void {
    (..., (void)&reflected_class_registrar<Registry, Entries>);
}

#endif

} // namespace detail

// =============================================================================
// virtual_ptr

//! Return the v-table pointer of an object (ADL customization point).
//!
//! This declaration is a catch-all that matches any argument list and returns
//! `void`, denoting the absence of customization. If an overload beats it for a
//! given argument type and registry, that overload is used to acquire a v-table
//! pointer instead of the registry's @ref policies::vptr policy.
//!
//! The library uses `boost_openmethod_vptr`, if found, when dispatching via a
//! @ref virtual_ parameter. It is not used by @ref virtual_ptr; wrapping an
//! object that has an overload in a @ref virtual_ptr is rejected at compile
//! time.
//!
//! @par Requirements
//!
//! The library uses argument-dependent lookup to find an overload that
//! satisfies the following requirements:
//!
//! @li The first parameter is a `const` lvalue reference to the virtual
//! argument.
//!
//! @li The second parameter is a pointer to a registry. Its role is to pass the
//! registry's *class* to the overload. It must not be dereferenced - its value
//! is `nullptr`. It may instead be `void*` if the overload does not need the
//! registry.
//!
//! @li The return type is @ref vptr_type.
//!
//! @par Example
//!
//! `Animal` carries its v-table pointer, and is registry-agnostic:
//!
//! include:../examples/virtual_.cpp#virtual_intrusive
//!
//! @see @ref inplace_vptr_base
//! @see [Virtual Pointer Alternatives](xref:ROOT:virtual_ptr_alt.adoc)
//! @see [Custom RTTI](xref:ROOT:custom_rtti.adoc)
void boost_openmethod_vptr(...);

namespace detail {

template<typename, class, typename = void>
struct is_smart_ptr_aux : std::false_type {};

template<typename Class, class Registry>
struct is_smart_ptr_aux<
    Class, Registry,
    std::void_t<
        typename virtual_traits<Class, Registry>::template rebind<Class>>> :
    std::true_type {};

template<class Class, class Other, class Registry, typename = void>
struct same_smart_ptr_aux : std::false_type {};

template<class Class, class Other, class Registry>
struct same_smart_ptr_aux<
    Class, Other, Registry,
    std::void_t<typename virtual_traits<Class, Registry>::template rebind<
        typename Other::element_type>>> :
    std::is_same<
        Other,
        typename virtual_traits<Class, Registry>::template rebind<
            typename Other::element_type>> {};

} // namespace detail

BOOST_OPENMETHOD_OPEN_NAMESPACE_DETAIL_UNLESS_MRDOCS

//! Test if argument is polymorphic (exposition only)
//!
//! Evaluates to `true` if `Class` is a polymorphic type, according to the
//! `rtti` policy of `Registry`.
//!
//! If Registry's `rtti` policy is std_rtti`, this is the same as
//! `std::is_polymorphic`. However, other `rtti` policies may have a different
//! view of what is polymorphic.
//!
//! @tparam Class A class type.
//! @tparam Registry A registry.
template<class Class, class Registry>
constexpr bool IsPolymorphic = Registry::rtti::template is_polymorphic<Class>;

//! Test if argument is a smart pointer (exposition only)
//!
//! Evaluates to `true` if `Class` is a smart pointer type, and false otherwise.
//! `Class` is considered a smart pointer if `virtual_traits<Class, Registry>`
//! exists and it defines a nested template `rebind<T>` that can be instantiated
//! with `Class`.
//!
//! @tparam Class A class type.
//! @tparam Registry A registry.
template<typename Class, class Registry>
constexpr bool IsSmartPtr = detail::is_smart_ptr_aux<Class, Registry>::value;

//! Test if arguments are same kind of smart pointers (exposition only)
//!
//! Evaluates to `true` if `Class` and `Other` are both smart pointers of the
//! same type.
//!
//! @tparam Class A class type.
//! @tparam Other Another class type.
//! @tparam Registry A registry.
template<class Class, class Other, class Registry>
constexpr bool SameSmartPtr =
    detail::same_smart_ptr_aux<Class, Other, Registry>::value;

BOOST_OPENMETHOD_CLOSE_NAMESPACE_DETAIL_UNLESS_MRDOCS

template<class Registry, typename Arg>
inline auto final_virtual_ptr(Arg&& obj);

namespace detail {

template<class Class, class Registry>
struct is_virtual<virtual_ptr<Class, Registry, void>> : std::true_type {};

template<class Class, class Registry>
struct is_virtual<virtual_ptr<Class, Registry, void>&> : std::true_type {};

template<class Class, class Registry>
struct is_virtual<const virtual_ptr<Class, Registry, void>&> :
    std::true_type {};

template<typename>
struct is_virtual_ptr_aux : std::false_type {};

template<class Class, class Registry>
struct is_virtual_ptr_aux<virtual_ptr<Class, Registry, void>> :
    std::true_type {};

template<class Class, class Registry>
struct is_virtual_ptr_aux<const virtual_ptr<Class, Registry, void>&> :
    std::true_type {};

template<typename T>
constexpr bool is_virtual_ptr = detail::is_virtual_ptr_aux<T>::value;

template<class Class, class Registry>
constexpr bool has_vptr_fn = std::is_same_v<
    decltype(boost_openmethod_vptr(
        std::declval<const Class&>(), std::declval<Registry*>())),
    vptr_type>;

BOOST_OPENMETHOD_DETAIL_HAS_STATIC_FN(vptr);

template<class Registry, class ArgType>
decltype(auto) acquire_vptr(const ArgType& arg) {
    // A class with a boost_openmethod_vptr overload does not need to be
    // wrapped: virtual_ptr and the hook fill the same goal, fast access
    // to the v-table pointer. The hook also returns the vptr by value,
    // which indirect registries cannot store (see box_vptr).
    static_assert(
        !has_vptr_fn<ArgType, Registry>,
        "do not wrap an object that has a boost_openmethod_vptr overload "
        "in a virtual_ptr; call methods directly on the object");

    Registry::require_initialized();

    if constexpr (has_vptr<
                      virtual_traits<const ArgType&, Registry>,
                      const ArgType&>) {
        return virtual_traits<const ArgType&, Registry>::vptr(arg);
    } else {
        return Registry::template policy<policies::vptr>::dynamic_vptr(arg);
    }
}

template<bool Indirect>
inline auto box_vptr(const vptr_type& vp) {
    if constexpr (Indirect) {
        return &vp;
    } else {
        return vp;
    }
}

inline auto unbox_vptr(vptr_type vp) {
    return vp;
}

inline auto unbox_vptr(const vptr_type* vpp) {
    return *vpp;
}

inline vptr_type null_vptr = nullptr;

} // namespace detail

//! Create a `virtual_ptr` for an object of a known exact class.
//!
//! Creates a @ref virtual_ptr to an object, setting its v-table pointer
//! according to the declared type of its argument. Assumes that the static and
//! dynamic types are the same. Sets the v-table pointer to the
//! @ref registry::static_vptr for the class.
//!
//! `Class` is _not_ required to be polymorphic.
//!
//! Nothing is looked up at runtime. Constructing a `virtual_ptr` from a
//! reference or a pointer reads the object's dynamic type through the
//! registry's `rtti` policy, then finds the v-table through its `vptr` policy;
//! here the v-table pointer is a static variable, read directly. It is also
//! the only way to create a `virtual_ptr` in a registry that uses
//! @ref policies::static_rtti, which has no dynamic type to consult and
//! disables the constructors that would need one.
//!
//! If runtime checks are enabled, and the argument is polymorphic, checks if
//! the static and dynamic types are the same. If not, calls the error handler
//! with a @ref final_error value, then terminates the program with `abort`.
//!
//! @par Errors
//!
//! @li @ref final_error The static and dynamic types of the object are
//! different.
//!
//! @par Example
//!
//! include:virtual_ptr.cpp#non_polymorphic_classes;final_virtual_ptr
//!
//! @tparam Registry A @ref registry.
//! @tparam Arg The type of the argument.
//! @param obj A reference to an object.
//! @return A `virtual_ptr<Class, Registry>` pointing to `obj`.
template<class Registry, typename Arg>
inline auto final_virtual_ptr(Arg&& obj) {
    using namespace detail;
    using VirtualPtr = virtual_ptr<std::remove_reference_t<Arg>, Registry>;
    using Traits = virtual_traits<Arg, Registry>;
    using Class = typename Traits::virtual_type;

    static_assert(!std::is_const_v<Class>);
    static_assert(!std::is_volatile_v<Class>);
    static_assert(!std::is_reference_v<Class>);
    static_assert(!std::is_pointer_v<Class>);

    Registry::require_initialized();

    if constexpr (
        Registry::has_runtime_checks &&
        Registry::rtti::template is_polymorphic<Class>) {

        // check that dynamic type == static type
        auto static_type = Registry::rtti::template static_type<Class>();
        auto dynamic_type = Registry::rtti::dynamic_type(Traits::peek(obj));

        // Equal type_ids settle it, and that is the common case: both come
        // from this module. Only when they differ is type_index needed, because
        // the same class has one type_id per module, so an object created
        // elsewhere yields a different type_id than this module's static_type
        // and a raw comparison alone would abort on a valid pointer. Ordering
        // the test this way keeps the check a pointer comparison except in the
        // cross-module case, where type_index may be as costly as comparing
        // names (see std_rtti::type_index).
        if (dynamic_type != static_type &&
            Registry::rtti::type_index(dynamic_type) !=
                Registry::rtti::type_index(static_type)) {
            if constexpr (is_not_void<typename Registry::error_handler>) {
                final_error error;
                error.static_type = static_type;
                error.dynamic_type = dynamic_type;
                Registry::error_handler::error(error);
            }

            abort();
        }
    }

    const vptr_type& vptr = Registry::template static_vptr<Class>;
    BOOST_ASSERT(vptr);

    return VirtualPtr(
        std::forward<Arg>(obj),
        detail::box_vptr<VirtualPtr::use_indirect_vptrs>(vptr));
}

//! Create a `virtual_ptr` for an object of a known exact class.
//!
//! This is an overload of `final_virtual_ptr` that uses the default
//! registry as the `Registry` template parameter.
//!
//! @par Example
//!
//! include:virtual_ptr.cpp#non_polymorphic_classes;final_virtual_ptr
//!
//! @see @ref final_virtual_ptr
// We could give a default value to Registry in the main template, but gcc
// doesn't like it.
template<class Arg>
inline auto final_virtual_ptr(Arg&& obj) {
    return final_virtual_ptr<BOOST_OPENMETHOD_DEFAULT_REGISTRY, Arg>(
        std::forward<Arg>(obj));
}
//! Wide pointer combining pointers to an object and its v-table
//!
//! A `virtual_ptr` is a wide pointer that combines pointers to an object and
//! its v-table. Calls to methods via `virtual_ptr` are as fast as ordinary
//! virtual function calls (typically two instructions).
//!
//! A `virtual_ptr` can be implicitly constructed from a reference, a pointer,
//! or another `virtual_ptr`, provided that they are type-compatible.
//!
//! `virtual_ptr` has specializations that use a `std::shared_ptr` or a
//! `std::unique_ptr` as the pointer to the object. The mechanism can be
//! extended to other smart pointers by specializing @ref virtual_traits. A
//! "plain" `virtual_ptr` can be constructed from a smart `virtual_ptr`, but not
//! the other way around.
//!
//! The default value for `Registry` can be customized by defining the
//! @ref BOOST_OPENMETHOD_DEFAULT_REGISTRY
//! preprocessor symbol.
//!
//! @par Requirements
//!
//! @li @ref virtual_traits must be specialized for @c Class&.
//! @li @c Class must be a class type, possibly cv-qualified, registered in
//! @c Registry.
//!
//! @tparam Class The class of the object, possibly cv-qualified
//! @tparam Registry The registry in which `Class` is registered
//! @tparam unnamed Implementation defined, use default
//!
//! @see [Smart Pointers](xref:ROOT:smart_pointers.adoc)
//! @see [Virtual Pointer Alternatives](xref:ROOT:virtual_ptr_alt.adoc)
//! @see [Performance](xref:ROOT:performance.adoc)
template<class Class, class Registry, typename>
class virtual_ptr {

    using traits = virtual_traits<Class&, Registry>;

#ifndef __MRDOCS__
    template<class, class, typename>
    friend class virtual_ptr;
    template<class, typename Arg>
    friend auto final_virtual_ptr(Arg&& obj);
#endif

    static constexpr bool is_smart_ptr = false;
    static constexpr bool use_indirect_vptrs = Registry::has_indirect_vptr;

    std::conditional_t<use_indirect_vptrs, const vptr_type*, vptr_type> vp;
    Class* obj;

    template<
        class Other,
        typename = std::enable_if_t<std::is_constructible_v<Class*, Other*>>>
    virtual_ptr(Other& other, decltype(vp) vp) : vp(vp), obj(&other) {
    }

  public:
    //! Class
    //!
    //! This is the same as `Class`.
    using element_type = Class;

    //! Default constructor
    //!
    //! @note This constructor does nothing. The state of the two pointers
    //! inside the object is as specified for uninitialized variables by C++.
    virtual_ptr() = default;

    //! Construct from `nullptr`
    //!
    //! Set both object and v-table pointers to `nullptr`.
    //!
    //! @param value A `nullptr`.
    //!
    //! @par Example
    //!
    //! include:virtual_ptr.cpp#ctor_nullptr
    //!
    //! @param value A `nullptr`.
    explicit virtual_ptr(std::nullptr_t) :
        vp(detail::box_vptr<use_indirect_vptrs>(detail::null_vptr)),
        obj(nullptr) {
    }

    //! Construct a `virtual_ptr` from a reference to an object
    //!
    //! The pointer to the v-table is obtained from @ref virtual_traits,
    //! if it provides a `vptr` function, or from the
    //! @ref policies::VptrFn::dynamic_vptr of the registry's `vptr`
    //! policy otherwise. An object with a @ref boost_openmethod_vptr
    //! overload is rejected at compile time: it carries its own v-table
    //! pointer, and does not need to be wrapped in a `virtual_ptr`.
    //!
    //! @param other A reference to a polymorphic object
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#ctor_ref
    //!
    //! @par Requirements
    //! @li @c Other must be a polymorphic class, according to the @c rtti
    //! policy of @c Registry.
    //! @li @c Other* must be constructible from @c Class*.
    //!
    //! @par Errors
    //!
    //! The following errors may occur, depending on the policies selected in
    //! `Registry`:
    //!
    //! @li @ref missing_class
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                IsPolymorphic<Other, Registry> &&
            std::is_constructible_v<Class*, Other*>>>
    virtual_ptr(Other& other) :
        vp(detail::box_vptr<use_indirect_vptrs>(
            detail::acquire_vptr<Registry>(other))),
        obj(&other) {
    }

    //! Construct a `virtual_ptr` from a pointer to an object
    //!
    //! The pointer to the v-table is obtained from @ref virtual_traits,
    //! if it provides a `vptr` function, or from the
    //! @ref policies::VptrFn::dynamic_vptr of the registry's `vptr`
    //! policy otherwise. An object with a @ref boost_openmethod_vptr
    //! overload is rejected at compile time: it carries its own v-table
    //! pointer, and does not need to be wrapped in a `virtual_ptr`.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#ctor_pointer
    //!
    //! @param other A pointer to a polymorphic object
    //!
    //! @par Requirements
    //!
    //! @li @c Other must be a polymorphic class, according to the @c rtti
    //! policy of @c Registry.
    //!
    //! @li @c Other* must be constructible from @c Class*.
    //!
    //! @par Errors
    //!
    //! The following errors may occur, depending on the policies selected in
    //! `Registry`:
    //!
    //! @li @ref missing_class
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                IsPolymorphic<Class, Registry> &&
            std::is_constructible_v<Class*, Other*>>>
    virtual_ptr(Other* other) :
        vp(detail::box_vptr<use_indirect_vptrs>(
            detail::acquire_vptr<Registry>(*other))),
        obj(other) {
    }

    //! Construct a `virtual_ptr` from another `virtual_ptr`
    //!
    //! Copy the object and v-table pointers from `other` to `this.
    //!
    //! `Other` is _not_ required to be a pointer to a polymorphic class.
    //!
    //! @par Examples
    //!
    //! Constructing from a plain `virtual_ptr`:
    //!
    //! include:virtual_ptr.cpp#non_polymorphic_classes;ctor_vptr
    //!
    //! Constructing from a smart `virtual_ptr`:
    //!
    //! include:virtual_ptr.cpp#ctor_shared_vptr
    //!
    //! No construction of a smart `virtual_ptr` from a plain `virtual_ptr`:
    //!
    //! include:virtual_ptr.cpp#ctor_shared_from_plain_rejected
    //!
    //! @param other A virtual_ptr to a type-compatible object
    //!
    //! @par Requirements
    //! @li @c Other's object pointer must be assignable to a @c Class*.
    template<
        class Other,
        typename = std::enable_if_t<std::is_constructible_v<
            Class*, typename virtual_ptr<Other, Registry>::element_type*>>>
    virtual_ptr(const virtual_ptr<Other, Registry>& other) :
        vp(other.vp), obj(other.get()) {
    }

    //! Assign a `virtual_ptr` from a reference to an object
    //!
    //! The pointer to the v-table is obtained from @ref virtual_traits,
    //! if it provides a `vptr` function, or from the
    //! @ref policies::VptrFn::dynamic_vptr of the registry's `vptr`
    //! policy otherwise. An object with a @ref boost_openmethod_vptr
    //! overload is rejected at compile time: it carries its own v-table
    //! pointer, and does not need to be wrapped in a `virtual_ptr`.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#assign_ref
    //!
    //! @param other A reference to a polymorphic object
    //!
    //! @par Requirements
    //!
    //! @li @c Other must be a polymorphic class, according to the @c rtti
    //! policy of @c Registry.
    //!
    //! @li @c Other* must be constructible from @c Class*.
    //!
    //! @par Errors
    //!
    //! The following errors may occur, depending on the policies selected in
    //! `Registry`:
    //!
    //! @li @ref missing_class
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                IsPolymorphic<Class, Registry> &&
            std::is_assignable_v<Class*&, Other*>>>
    virtual_ptr& operator=(Other& other) {
        obj = &other;
        vp = detail::box_vptr<use_indirect_vptrs>(
            detail::acquire_vptr<Registry>(other));
        return *this;
    }

    //! Assign a `virtual_ptr` from a pointer to an object
    //!
    //! The pointer to the v-table is obtained from @ref virtual_traits,
    //! if it provides a `vptr` function, or from the
    //! @ref policies::VptrFn::dynamic_vptr of the registry's `vptr`
    //! policy otherwise. An object with a @ref boost_openmethod_vptr
    //! overload is rejected at compile time: it carries its own v-table
    //! pointer, and does not need to be wrapped in a `virtual_ptr`.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#assign_pointer
    //!
    //! @param other A pointer to a polymorphic object
    //!
    //! @par Requirements
    //! @li @c Other must be a polymorphic class, according to the @c rtti
    //! policy of @c Registry.
    //! @li @c Other* must be constructible from @c Class*.
    //!
    //! @par Errors
    //!
    //! The following errors may occur, depending on the policies selected in
    //! `Registry`:
    //!
    //! @li @ref missing_class
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                IsPolymorphic<Class, Registry> &&
            std::is_assignable_v<Class*&, Other*>>>
    virtual_ptr& operator=(Other* other) {
        obj = other;
        vp = detail::box_vptr<use_indirect_vptrs>(
            detail::acquire_vptr<Registry>(*other));
        return *this;
    }

    //! Assign a `virtual_ptr` from another `virtual_ptr`
    //!
    //! Copy the object and v-table pointers from `other` to `this.
    //!
    //! `Other` is _not_ required to be a pointer to a polymorphic class.
    //!
    //! @par Examples
    //!
    //! Assigning from a plain `virtual_ptr`:
    //!
    //! include:virtual_ptr.cpp#non_polymorphic_classes;assign_vptr
    //!
    //! Assigning from a smart `virtual_ptr`:
    //!
    //! include:virtual_ptr.cpp#assign_shared_vptr
    //!
    //! No assignment from a plain `virtual_ptr` to a smart `virtual_ptr`:
    //!
    //! include:virtual_ptr.cpp#assign_shared_from_plain_rejected
    //!
    //! @param other A virtual_ptr to a type-compatible object
    //!
    //! @par Requirements
    //! @li @c Other's object pointer must be assignable to a @c Class*.
    template<
        class Other,
        typename = std::enable_if_t<std::is_assignable_v<
            Class*&, typename virtual_ptr<Other, Registry>::element_type*>>>
    virtual_ptr& operator=(const virtual_ptr<Other, Registry>& other) {
        obj = other.get();
        vp = other.vp;
        return *this;
    }

    //! Set a `virtual_ptr` to `nullptr`
    //!
    //! Set both object and v-table pointers to `nullptr`.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#assign_nullptr
    virtual_ptr& operator=(std::nullptr_t) {
        obj = nullptr;
        vp = detail::box_vptr<use_indirect_vptrs>(detail::null_vptr);
        return *this;
    }

    //! Get a pointer to the object
    //!
    //! @return A pointer to the object
    auto get() const -> Class* {
        return obj;
    }

    //! Get a pointer to the object
    //!
    //! @return A pointer to the object
    auto operator->() const {
        return get();
    }

    //! Get a reference to the object
    //!
    //! @return A reference to the object
    auto operator*() const -> element_type& {
        return *get();
    }

    //! Get a pointer to the object
    //!
    //! @return A pointer to the object
    auto pointer() const -> element_type* {
        return obj;
    }

    //! Cast to another `virtual_ptr` type
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#cast
    //!
    //! @tparam Other The target class of the cast
    //! @return A `virtual_ptr<Other, Registry>` pointing to the same object
    //! @par Requirements
    //! @li @c Other must be a base or derived class of @c Class.
    template<
        class Other,
        typename = std::enable_if_t<
            std::is_base_of_v<element_type, Other> ||
            std::is_base_of_v<Other, element_type>>>
    auto cast() const -> decltype(auto) {
        return virtual_ptr<Other, Registry>(
            traits::template cast<Other&>(*obj), vp);
    }

    //! Construct a `virtual_ptr` for an object of a known exact class
    //!
    //! This function forwards to @ref final_virtual_ptr.
    //!
    //! @tparam Other The type of the argument
    //! @param obj A reference to an object
    //! @return A `virtual_ptr<Class, Registry>` pointing to `obj`
    template<class Other>
    static auto final(Other&& obj) {
        return final_virtual_ptr<Registry>(std::forward<Other>(obj));
    }

    //! Get the v-table pointer
    //! @return The v-table pointer
    auto vptr() const {
        return detail::unbox_vptr(this->vp);
    }
};

//! Wide pointer combining a smart pointer to an object and a pointer to its
//! v-table
//!
//! This specialization of `virtual_ptr` uses a smart pointer to track the
//! object, instead of a plain pointer.
//!
//! @tparam SmartPtr A smart pointer type
//! @tparam Registry The registry in which the underlying class is registered
//!
//! @see [Smart Pointers](xref:ROOT:smart_pointers.adoc)
template<class SmartPtr, class Registry>
class virtual_ptr<
    SmartPtr, Registry,
    std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                         IsSmartPtr<SmartPtr, Registry>>> {

#ifndef __MRDOCS__
    template<class, class, typename>
    friend class virtual_ptr;
    template<class, typename Arg>
    friend auto final_virtual_ptr(Arg&& obj);
#endif

    static constexpr bool is_smart_ptr = true;
    static constexpr bool use_indirect_vptrs = Registry::has_indirect_vptr;

    using traits = virtual_traits<SmartPtr, Registry>;

    std::conditional_t<use_indirect_vptrs, const vptr_type*, vptr_type> vp;
    SmartPtr obj;

    template<
        class Other,
        typename = std::enable_if_t<std::is_constructible_v<SmartPtr*, Other*>>>
    virtual_ptr(Other& other, decltype(vp) vp) : vp(vp), obj(&other) {
    }

    template<typename Arg>
    virtual_ptr(Arg&& obj, decltype(vp) vp) :
        vp(vp), obj(std::forward<Arg>(obj)) {
    }

  public:
    //! Class pointed to by SmartPtr
    using element_type = typename SmartPtr::element_type;

    //! Default constructor
    //!
    //! Construct the object pointer using its default constructor. Set the
    //! v-table pointer to `nullptr`.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#non_polymorphic_classes;shared_ctor_default
    virtual_ptr() :
        vp(detail::box_vptr<use_indirect_vptrs>(detail::null_vptr)) {
    }

    //! Construct from `nullptr`
    //!
    //! Construct the object pointer using its default constructor. Set the
    //! v-table pointer to `nullptr`.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#non_polymorphic_classes;shared_ctor_nullptr
    //!
    //! @param value A `nullptr`.
    explicit virtual_ptr(std::nullptr_t) :
        vp(detail::box_vptr<use_indirect_vptrs>(detail::null_vptr)) {
    }

    virtual_ptr(const virtual_ptr& other) = default;

    virtual_ptr(virtual_ptr&& other) :
        vp(std::exchange(
            other.vp, detail::box_vptr<use_indirect_vptrs>(detail::null_vptr))),
        obj(std::move(other.obj)) {
    }

    //! Construct from a (const) smart pointer to a derived class
    //!
    //! Set the object pointer with a copy of `other`. Set the v-table pointer
    //! according to the dynamic type of `*other`.
    //!
    //! @par Examples
    //!
    //! Constructing from a `std::shared_ptr`:
    //!
    //! include:virtual_ptr.cpp#shared_ctor_const_smart_ptr
    //!
    //! A move-only smart pointer cannot be copied from. Use the move
    //! constructor instead:
    //!
    //! include:virtual_ptr.cpp#unique_copy_rejected
    //!
    //! @par Requirements
    //! @li @c SmartPtr and @c Other must be instantiated from the same template -
    //! e.g. both @c std::shared_ptr or both @c std::unique_ptr.
    //! @li @c Other must be a smart pointer to a polymorphic class derived from
    //! @c element_type.
    //! @li @c SmartPtr must be constructible from @c const @c Other&.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                SameSmartPtr<SmartPtr, Other, Registry> &&
            std::is_constructible_v<SmartPtr, const Other&>>,
        typename = std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(
            detail::) IsPolymorphic<typename Other::element_type, Registry>>>
    virtual_ptr(const Other& other) :
        vp(detail::box_vptr<use_indirect_vptrs>(
            other ? detail::acquire_vptr<Registry>(*other)
                  : detail::null_vptr)),
        obj(other) {
    }

    //! Construct from a smart pointer to a derived class
    //!
    //! Copy object pointer from `other` to `this`. Set the v-table pointer
    //! according to the dynamic type of `*other`.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#shared_ctor_smart_ptr
    //!
    //! @par Requirements
    //! @li @c SmartPtr and @c Other must be instantiated from the same template -
    //! e.g. both @c std::shared_ptr or both @c std::unique_ptr.
    //! @li @c Other must be a smart pointer to a polymorphic class derived from
    //! @c element_type.
    //! @li @c SmartPtr must be constructible from @c Other&.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                SameSmartPtr<SmartPtr, Other, Registry> &&
            std::is_constructible_v<SmartPtr, Other&>>,
        typename = std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(
            detail::) IsPolymorphic<typename Other::element_type, Registry>>>
    virtual_ptr(Other& other) :
        vp(detail::box_vptr<use_indirect_vptrs>(
            other ? detail::acquire_vptr<Registry>(*other)
                  : detail::null_vptr)),
        obj(other) {
    }

    //! Move-construct from a smart pointer to a derived class
    //!
    //! Move object pointer from `other` to `this`. Set the v-table pointer
    //! according to the dynamic type of `*other`.
    //!
    //! @par Examples
    //!
    //! Move-constructing from a `std::shared_ptr`:
    //!
    //! include:virtual_ptr.cpp#shared_ctor_move_smart_ptr
    //!
    //! Move-constructing from a `std::unique_ptr`:
    //!
    //! include:virtual_ptr.cpp#unique_ctor_move_smart_ptr
    //!
    //! @par Requirements
    //! @li @c SmartPtr and @c Other must be instantiated from the same template -
    //! e.g. both @c std::shared_ptr or both @c std::unique_ptr.
    //! @li @c Other must be a smart pointer to a polymorphic class derived from
    //! @c element_type.
    //! @li @c SmartPtr must be constructible from @c Other&&.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                SameSmartPtr<SmartPtr, Other, Registry> &&
            std::is_constructible_v<SmartPtr, Other&&>>,
        typename = std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(
            detail::) IsPolymorphic<typename Other::element_type, Registry>>>
    virtual_ptr(Other&& other) :
        vp(detail::box_vptr<use_indirect_vptrs>(
            other ? detail::acquire_vptr<Registry>(*other)
                  : detail::null_vptr)),
        obj(std::move(other)) {
    }

    //! Construct from a smart virtual (const) pointer to a derived class
    //!
    //! Copy the object and v-table pointers from `other`.
    //!
    //! `Other` is _not_ required to be a pointer to a polymorphic class.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#non_polymorphic_classes;shared_ctor_const_vptr
    //!
    //! @par Requirements
    //! @li @c SmartPtr and @c Other must be instantiated from the same template -
    //! e.g. both @c std::shared_ptr or both @c std::unique_ptr.
    //! @li @c Other must be a virtual pointer to a class derived from
    //! @c element_type.
    //! @li @c SmartPtr must be constructible from @c Other&.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                SameSmartPtr<SmartPtr, Other, Registry> &&
            std::is_constructible_v<SmartPtr, const Other&>>>
    virtual_ptr(const virtual_ptr<Other, Registry>& other) :
        vp(other.vp), obj(other.obj) {
    }

    //! Construct-move from a virtual pointer to a derived class
    //!
    //! Move the object pointer from `other` to `this`. Copy the v-table pointer
    //! from `other`.
    //!
    //! `Other` is _not_ required to be a pointer to a polymorphic class.
    //!
    //! @par Examples
    //!
    //! Move-constructing from a shared `virtual_ptr`:
    //!
    //! include:virtual_ptr.cpp#non_polymorphic_classes;shared_ctor_move_vptr
    //!
    //! Move-constructing from a unique `virtual_ptr`:
    //!
    //! include:virtual_ptr.cpp#unique_ctor_move_vptr
    //!
    //! @par Requirements
    //! @li @c SmartPtr and @c Other must be instantiated from the same template -
    //! e.g. both @c std::shared_ptr or both @c std::unique_ptr.
    //! @li @c Other must be a smart pointer to a class derived from
    //! @c element_type.
    //! @li @c SmartPtr must be constructible from @c Other&&.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                SameSmartPtr<SmartPtr, Other, Registry> &&
            std::is_constructible_v<SmartPtr, Other&&>>>
    virtual_ptr(virtual_ptr<Other, Registry>&& other) :
        vp(std::exchange(
            other.vp, detail::box_vptr<use_indirect_vptrs>(detail::null_vptr))),
        obj(std::move(other.obj)) {
    }

    //! Assign from `nullptr`
    //!
    //! Reset the object pointer using its default constructor. Set the
    //! v-table pointer to `nullptr`.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#non_polymorphic_classes;shared_assign_nullptr
    //!
    //! @param value A `nullptr`.
    virtual_ptr& operator=(std::nullptr_t) {
        obj = SmartPtr();
        vp = detail::box_vptr<use_indirect_vptrs>(detail::null_vptr);
        return *this;
    }

    //! Assign from a (const) smart pointer to a derived class
    //!
    //! Copy the object pointer from `other` to `this`. Set the v-table pointer
    //! according to the dynamic type of `*other`.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#shared_assign_smart_ptr
    //!
    //! @par Requirements
    //! @li @c SmartPtr and @c Other must be instantiated from the same template -
    //! e.g. both @c std::shared_ptr or both @c std::unique_ptr.
    //! @li @c Other must be a smart pointer to a polymorphic class derived from
    //! @c element_type.
    //! @li @c SmartPtr must be constructible from @c const @c Other&.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                SameSmartPtr<SmartPtr, Other, Registry> &&
            std::is_assignable_v<SmartPtr, const Other&>>,
        typename = std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(
            detail::) IsPolymorphic<typename Other::element_type, Registry>>>
    virtual_ptr& operator=(const Other& other) {
        obj = other;
        vp = detail::box_vptr<use_indirect_vptrs>(
            detail::acquire_vptr<Registry>(*other));
        return *this;
    }

    //! Move-assign from a smart pointer to a derived class
    //!
    //! Move object pointer from `other` to `this`. Set the v-table pointer
    //! according to the dynamic type of `*other`.
    //!
    //! @par Examples
    //!
    //! Move-assigning from a `std::shared_ptr`:
    //!
    //! include:virtual_ptr.cpp#shared_assign_move_smart_ptr
    //!
    //! Move-assigning from a `std::unique_ptr`:
    //!
    //! include:virtual_ptr.cpp#unique_assign_move_smart_ptr
    //!
    //! @par Requirements
    //! @li @c SmartPtr and @c Other must be instantiated from the same template -
    //! e.g. both @c std::shared_ptr or both @c std::unique_ptr.
    //! @li @c Other must be a smart pointer to a polymorphic class derived from
    //! @c element_type.
    //! @li @c SmartPtr must be constructible from @c Other&&.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                SameSmartPtr<SmartPtr, Other, Registry> &&
            std::is_assignable_v<SmartPtr, Other&&>>,
        typename = std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(
            detail::) IsPolymorphic<typename Other::element_type, Registry>>>
    virtual_ptr& operator=(Other&& other) {
        vp = detail::box_vptr<use_indirect_vptrs>(
            other ? detail::acquire_vptr<Registry>(*other) : detail::null_vptr);
        obj = std::move(other);
        return *this;
    }

    //! Assign from a smart virtual pointer to a derived class
    //!
    //! Copy the object and v-table pointers from `other` to `this`.
    //!
    //! `Other` is _not_ required to be a pointer to a polymorphic class.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#non_polymorphic_classes;shared_assign_vptr
    //!
    //! @par Requirements
    //! @li @c SmartPtr and @c Other must be instantiated from the same template -
    //! e.g. both @c std::shared_ptr or both @c std::unique_ptr.
    //! @li @c Other must be a virtual pointer to a class derived from
    //! @c element_type.
    //! @li @c SmartPtr must be constructible from @c Other&.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                SameSmartPtr<SmartPtr, Other, Registry> &&
            std::is_assignable_v<SmartPtr, Other&>>>
    virtual_ptr& operator=(virtual_ptr<Other, Registry>& other) {
        obj = other.obj;
        vp = other.vp;
        return *this;
    }

    virtual_ptr& operator=(const virtual_ptr& other) = default;

    //! Assign from a smart virtual const pointer to a derived class
    //!
    //! Copy the object and v-table pointers from `other` to `this`.
    //!
    //! `Other` is _not_ required to be a pointer to a polymorphic class.
    //!
    //! @par Example
    //! include:virtual_ptr.cpp#non_polymorphic_classes;shared_assign_const_vptr
    //!
    //! @par Requirements
    //! @li @c SmartPtr and @c Other must be instantiated from the same template -
    //! e.g. both @c std::shared_ptr or both @c std::unique_ptr.
    //! @li @c Other must be a virtual pointer to a class derived from
    //! @c element_type.
    //! @li @c SmartPtr must be constructible from @c Other&.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                SameSmartPtr<SmartPtr, Other, Registry> &&
            std::is_assignable_v<SmartPtr, const Other&>>>
    virtual_ptr& operator=(const virtual_ptr<Other, Registry>& other) {
        obj = other.obj;
        vp = other.vp;
        return *this;
    }

    //! Move from a virtual pointer to a derived class
    //!
    //! Move the object pointer from `other` to `this`. Copy the v-table pointer
    //! from `other`.
    //!
    //! `Other` is _not_ required to be a pointer to a polymorphic class.
    //!
    //! @par Examples
    //!
    //! Move-assigning from a shared `virtual_ptr`:
    //!
    //! include:virtual_ptr.cpp#non_polymorphic_classes;shared_assign_move_vptr
    //!
    //! Move-assigning from a unique `virtual_ptr`:
    //!
    //! include:virtual_ptr.cpp#unique_assign_move_vptr
    //!
    //! @par Requirements
    //! @li @c SmartPtr and @c Other must be instantiated from the same template -
    //! e.g. both @c std::shared_ptr or both @c std::unique_ptr.
    //! @li @c Other must be a smart pointer to a class derived from
    //! @c element_type.
    //! @li @c SmartPtr must be constructible from @c Other&&.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                SameSmartPtr<SmartPtr, Other, Registry> &&
            std::is_assignable_v<SmartPtr, Other&&>>>
    virtual_ptr& operator=(virtual_ptr<Other, Registry>&& other) {
        vp = std::exchange(
            other.vp, detail::box_vptr<use_indirect_vptrs>(detail::null_vptr));
        obj = std::move(other.obj);

        return *this;
    }

    //! Get a pointer to the object
    //!
    //! @return A *plain* pointer to the object
    auto get() const -> element_type* {
        return obj.get();
    }

    //! Get a pointer to the object
    //!
    //! @return A *plain* pointer to the object
    auto operator->() const -> element_type* {
        return get();
    }

    //! Get a reference to the object
    //!
    //! @return A reference to the object
    auto operator*() const -> element_type& {
        return *get();
    }

    //! Get a smart pointer to the object
    //!
    //! @return A const reference to the object pointer
    auto pointer() const -> const SmartPtr& {
        return obj;
    }

    //! Cast to another `virtual_ptr` type
    //! @tparam Other The target class of the cast
    //! @return A `virtual_ptr<Other, Registry>` pointing to the same object
    //! @par Requirements
    //! @li @c Other must be a base or a derived class of @c Class.
    template<
        class Other,
        typename = std::enable_if_t<
            std::is_base_of_v<element_type, Other> ||
            std::is_base_of_v<Other, element_type>>>
    auto cast() & -> decltype(auto) {
        using other_smart_ptr = typename traits::template rebind<Other>;

        return virtual_ptr<other_smart_ptr, Registry>(
            traits::template cast<other_smart_ptr>(obj), vp);
    }

    template<
        class Other,
        typename = std::enable_if_t<
            std::is_base_of_v<element_type, Other> ||
            std::is_base_of_v<Other, element_type>>>
    auto cast() const& -> decltype(auto) {
        using other_smart_ptr = typename traits::template rebind<Other>;

        return virtual_ptr<other_smart_ptr, Registry>(
            traits::template cast<other_smart_ptr>(obj), vp);
    }

    template<class Other>
    auto cast() && -> decltype(auto) {
        static_assert(
            std::is_base_of_v<element_type, Other> ||
            std::is_base_of_v<Other, element_type>);

        using other_smart_ptr = typename traits::template rebind<Other>;

        return virtual_ptr<other_smart_ptr, Registry>(
            traits::template cast<other_smart_ptr>(std::move(obj)), vp);
    }

    //! Construct a `virtual_ptr` from a smart pointer to an object of a known
    //! exact class
    //!
    //! This function forwards to @ref final_virtual_ptr.
    //!
    //! @tparam Other The type of the argument
    //! @param obj A reference to an object
    //! @return A `virtual_ptr<Class, Registry>` pointing to `obj`
    template<class Other>
    static auto final(Other&& obj) {
        return final_virtual_ptr<Registry>(std::forward<Other>(obj));
    }

    //! Get the v-table pointer
    //! @return The v-table pointer
    auto vptr() const {
        return detail::unbox_vptr(this->vp);
    }
};

//! Construct a `virtual_ptr` from a lvalue reference.
//!
//! @tparam Class A class type, possibly cv-qualified.
//! @param obj A lvalue reference to an object.
//! @return A `virtual_ptr<Class>`.
template<class Class>
virtual_ptr(Class& obj)
    -> virtual_ptr<Class, BOOST_OPENMETHOD_DEFAULT_REGISTRY>;

//! Construct a `virtual_ptr` from a xvalue reference.
//!
//! @tparam Class A class type.
//! @param obj A xvalue reference to an object.
//! @return A `virtual_ptr<Class>`.
template<class Class>
virtual_ptr(Class&& obj)
    -> virtual_ptr<Class, BOOST_OPENMETHOD_DEFAULT_REGISTRY>;

// Alas this is not allowed:
// template<class Registry, class Class>
// virtual_ptr<Registry>(Class&) -> virtual_ptr<Class, Registry>;

//! Compare two `virtual_ptr`s for equality.
//!
//! Compare the underlying object pointers for equality. The v-table pointers
//! are not compared.
//!!
//! @tparam Left The type of the left-hand side argument.
//! @tparam Right The type of the right-hand side argument.
//! @tparam Registry A @ref registry.
//! @param left A reference to a `virtual_ptr`.
//! @param right A reference to a `virtual_ptr`.
//! @return `true` if both `virtual_ptr`s point to the same object or both
//! are `nullptr`, `false` otherwise.
template<class Left, class Right, class Registry>
auto operator==(
    const virtual_ptr<Left, Registry>& left,
    const virtual_ptr<Right, Registry>& right) -> bool {
    return left.pointer() == right.pointer();
}

//! Compare two `virtual_ptr`s for inequality.
//!
//! Compare the underlying object pointers for inequality. The v-table pointers
//! are not compared.
//!! @tparam Left The type of the left-hand side argument.
//! @tparam Right The type of the right-hand side argument.
//! @tparam Registry A @ref registry.
//! @param left A reference to a `virtual_ptr`.
//! @param right A reference to a `virtual_ptr`.
//! @return `true` if both `virtual_ptr`s point to different objects, or one
//! is `nullptr` and the other is not, `false` otherwise.
template<class Left, class Right, class Registry>
auto operator!=(
    const virtual_ptr<Left, Registry>& left,
    const virtual_ptr<Right, Registry>& right) -> bool {
    return !(left == right);
}

//! Specialize virtual_traits for `virtual_ptr`.
//!
//! Specialize virtual_traits for `virtual_ptr`\'s passed by value.
//!
//! @tparam Class A class type, possibly cv-qualified.
//! @tparam Registry A @ref registry.
template<class Class, class Registry>
struct virtual_traits<virtual_ptr<Class, Registry>, Registry> {
    //! `Class`, stripped from cv-qualifiers.
    using virtual_type =
        std::remove_cv_t<typename virtual_ptr<Class, Registry>::element_type>;

    //! Return a reference to a non-modifiable `Class` object.
    //! @param arg A reference to a non-modifiable `Class` object.
    //! @return A reference to the same object.
    static auto peek(const virtual_ptr<Class, Registry>& ptr)
        -> const virtual_ptr<Class, Registry>& {
        return ptr;
    }

    //! Cast to another type.
    //!
    //! Cast a `virtual_ptr` to another type, using its `cast` member function.
    //!
    //! @param obj A lvalue reference to a `virtual_ptr`.
    //! @return A lvalue reference to a `virtual_ptr` to the same object, cast
    //! to `Derived::element_type`.
    template<typename Derived>
    static auto cast(const virtual_ptr<Class, Registry>& ptr)
        -> decltype(auto) {
        return ptr.template cast<typename Derived::element_type>();
    }

    //! Cast to another type.
    //!
    //! Cast a `virtual_ptr` to another type, using its `cast` member function.
    //!
    //! @param obj A xvalue reference to a `virtual_ptr`.
    //! @return A xvalue reference to a `virtual_ptr` to the same object, cast
    //! to `Derived::element_type`.
    template<typename Derived>
    static auto cast(virtual_ptr<Class, Registry>&& ptr) -> decltype(auto) {
        return std::move(ptr).template cast<typename Derived::element_type>();
    }
};

//! Specialize virtual_traits for `virtual_ptr`.
//!
//! Specialize virtual_traits for `virtual_ptr`\'s passed by const reference.
//!
//! @tparam Class A class type, possibly cv-qualified.
//! @tparam Registry A @ref registry.
template<class Class, class Registry>
struct virtual_traits<const virtual_ptr<Class, Registry>&, Registry> {
    //! `Class`, stripped from cv-qualifiers.
    using virtual_type =
        std::remove_cv_t<typename virtual_ptr<Class, Registry>::element_type>;

    //! Return a reference to a non-modifiable `Class` object.
    //! @param arg A reference to a non-modifiable `Class` object.
    //! @return A reference to the same object.
    static auto peek(const virtual_ptr<Class, Registry>& ptr)
        -> const virtual_ptr<Class, Registry>& {
        return ptr;
    }

    //! Cast to another type.
    //!
    //! Cast a `virtual_ptr` to another type, using its `cast` member function.
    //!
    //! @param obj A lvalue reference to a `virtual_ptr`.
    //! @return A lvalue reference to a `virtual_ptr` to the same object, cast
    //! to `Derived::element_type`.
    template<typename Derived>
    static auto cast(const virtual_ptr<Class, Registry>& ptr)
        -> decltype(auto) {
        return ptr.template cast<
            typename std::remove_reference_t<Derived>::element_type>();
    }
};

// =============================================================================
// Method

namespace detail {

template<typename P, typename Q, class Registry>
struct select_overrider_virtual_type_aux {
    using type = void;
};

template<typename P, typename Q, class Registry>
struct select_overrider_virtual_type_aux<virtual_<P>, Q, Registry> {
    using type = virtual_type<Q, Registry>;
};

template<typename P, typename Q, class Registry>
struct select_overrider_virtual_type_aux<
    virtual_ptr<P, Registry>, virtual_ptr<Q, Registry>, Registry> {
    using type = typename virtual_traits<
        virtual_ptr<Q, Registry>, Registry>::virtual_type;
};

template<typename P, typename Q, class Registry>
struct select_overrider_virtual_type_aux<
    const virtual_ptr<P, Registry>&, const virtual_ptr<Q, Registry>&,
    Registry> {
    using type = typename virtual_traits<
        const virtual_ptr<Q, Registry>&, Registry>::virtual_type;
};

template<typename P, typename Q, class Registry>
using select_overrider_virtual_type =
    typename select_overrider_virtual_type_aux<P, Q, Registry>::type;

template<
    typename MethodParameters, typename OverriderParameters, class Registry>
using overrider_virtual_types = boost::mp11::mp_remove<
    boost::mp11::mp_transform_q<
        boost::mp11::mp_bind_back<select_overrider_virtual_type, Registry>,
        MethodParameters, OverriderParameters>,
    void>;

template<class Method, class Rtti, std::size_t Index>
struct init_bad_call {
    template<typename Arg, typename... Args>
    static auto fn(bad_call& error, const Arg& arg, const Args&... args) {
        if constexpr (Index == 0u) {
            error.method = Rtti::template static_type<Method>();
            error.arity = sizeof...(args) + 1;
        }

        type_id arg_type_id;

        if constexpr (is_virtual_ptr<Arg>) {
            arg_type_id = Rtti::dynamic_type(*arg);
        } else {
            arg_type_id = Rtti::dynamic_type(arg);
        }

        error.types[Index] = arg_type_id;

        init_bad_call<Method, Rtti, Index + 1>::fn(error, args...);
    }

    static auto fn(bad_call&) {
    }
};

template<class Method, class Rtti>
struct init_bad_call<Method, Rtti, bad_call::max_types> {
    static auto fn(bad_call&) {
    }
};

template<class Registry>
using method_base = std::conditional_t<
    Registry::has_deferred_static_rtti, deferred_method_info, method_info>;

template<typename T, class Registry>
struct parameter_traits {
    static auto peek(const T& value) -> const T& {
        return value;
    }

    template<typename>
    static auto cast(T value) -> T {
        return value;
    }
};

template<typename T, class Registry>
struct parameter_traits<virtual_<T>, Registry> : virtual_traits<T, Registry> {};

template<class Class, class Registry>
struct parameter_traits<virtual_ptr<Class, Registry, void>, Registry> :
    virtual_traits<virtual_ptr<Class, Registry, void>, Registry> {};

template<class Class, class Registry>
struct parameter_traits<const virtual_ptr<Class, Registry, void>&, Registry> :
    virtual_traits<const virtual_ptr<Class, Registry, void>&, Registry> {};

template<typename...>
constexpr bool false_t = false; // workaround before CWG2518/P2593R1

template<typename T, class Registry, typename = void>
struct validate_method_parameter : std::true_type {};

template<typename T, class Registry, typename U>
struct validate_method_parameter<virtual_<T>, Registry, U> : std::false_type {
    static_assert(false_t<T>, "virtual_traits not specialized for type");
};

template<typename T, class Registry>
struct validate_method_parameter<
    virtual_<T>, Registry,
    std::void_t<typename virtual_traits<T, Registry>::virtual_type>> :
    std::bool_constant<
        has_vptr_fn<virtual_type<T, Registry>, Registry> ||
        Registry::rtti::template is_polymorphic<virtual_type<T, Registry>>> {
    static_assert(
        validate_method_parameter::value,
        "virtual_<> parameter is not a polymorphic class and no "
        "boost_openmethod_vptr is applicable");
};

template<class Class, class Registry>
struct validate_method_parameter<virtual_ptr<Class, Registry>, Registry, void> :
    std::true_type {};

template<class Class, class Registry, class MethodRegistry>
struct validate_method_parameter<
    virtual_ptr<Class, Registry>, MethodRegistry, void> : std::false_type {
    static_assert(
        false_t<Class, Registry, MethodRegistry>, "registry mismatch");
};
} // namespace detail

//! Implement a method
//!
//! Methods are created by specializing the `method` class template with an
//! identifier, a function type and optionally a registry.
//!
//! `Id` is a type, typically an incomplete class declaration named after the
//! method's purpose. It is used to allow different methods with the same
//! signature.
//!
//! `Fn` is a function type, i.e. a type in the form `ReturnType(Parameters...)`.
//!
//! `Registry` is an instantiation of class template @ref registry. Methods may
//! use only classes that have been registered in the same registry as virtual
//! parameters and arguments. The registry also contains a set of policies that
//! influence several aspects of the dispatch mechanism - for example, how to
//! acquire a v-table pointer for an object, how to report errors, whether to
//! perform sanity checks, etc.
//!
//! The default value for `Registry` is @ref default_registry, but it can be
//! overridden by defining the preprocessor symbol
//! @ref BOOST_OPENMETHOD_DEFAULT_REGISTRY, *before* including
//! `<boost/openmethod/core.hpp>` (or any header that includes it, like
//! `<boost/openmethod.hpp>`). Setting the symbol afterwards has no effect.
//!
//! Specializations of `method` have a single instance: the static member `fn`,
//! which has an `operator()` that forwards to the appropriate overrider. It is
//! selected in the same way as overloaded function resolution:
//!
//! 1. Form the set of all applicable overriders. An overrider is applicable
//!    if it can be called with the arguments passed to the method.
//!
//! 2. If the set is empty, call the error handler (if present in the
//!    registry), then terminate the program with `abort`.
//!
//! 3. Remove the overriders that are dominated by other overriders in the set.
//!    Overrider A dominates overrider B if at least one of its virtual formal
//!    parameters is more specialized than B's, and if none of B's virtual
//!    parameters is more specialized than A's.
//!
//! 4. If the resulting set contains exactly one overrider, call it.
//!
//! If a single most specialized overrider does not exist, the program is
//! terminated via `abort`. If the registry contains an @ref error_handler
//! policy, its `error` function is called with an object that describes the
//! error, prior calling `abort`. `error` may prevent termination by throwing an
//! exception.
//!
//! For each virtual argument `arg`, the dispatch mechanism calls
//! `virtual_traits::peek(arg)` and deduces the v-table pointer from the
//! `result`, using the first of the following methods that applies:
//!
//! 1. If `result` is a `virtual_ptr`, get the pointer to the v-table from it.
//!
//! 2. If @ref boost_openmethod_vptr can be called with `result` and a
//!    `Registry*`, and it returns a `vptr_type`, call it.
//!
//! 3. If @ref virtual_traits provides a `vptr` function, call it.
//!
//! 4. Call the @ref policies::VptrFn::dynamic_vptr of the registry's `vptr`
//!    policy.
//!
//! @par N2216 Handling of Ambiguous Calls
//!
//! If `Registry` was initialized with the @ref n2216 option, ambiguous calls
//! are not an error. Instead, the following extra steps are taken to select an
//! overrider:
//!
//! 1. If the return type is a registered polymorphic type, remove all the
//!    overriders that return a less specific type than others.
//!
//! 2. If the resulting set contains only one overrider, call it.
//!
//! 3. Otherwise, call one of the remaining overriders. Which overrider is
//!    selected is not specified, but it is the same across calls with the
//!    same arguments types.
//!
//! @tparam Id A type
//! @tparam Fn A function type
//! @tparam Registry The registry in which the method is defined
//!
//! @see [Core API](xref:ROOT:core_api.adoc)
template<
    typename Id, typename Fn,
    class Registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY>
class method;

//! Method with a specific id, signature and return type
//!
//! `method` implements an open-method that takes a parameter list -
//! `Parameters` - and returns a `ReturnType`.
//!
//! `Parameters` must contain at least one virtual parameter, i.e. a parameter
//! that has a type in the form `virtual_ptr<T, Registry>` or `virtual\_<T>`.
//! The dynamic types of the virtual arguments are taken into account to select
//! the overrider to call.
//!
//! @see method
//!
//! @tparam Id A type representing the method's name
//! @tparam ReturnType The return type of the method
//! @tparam Parameters The types of the parameters
//! @tparam Registry The registry of the method
template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
class method<Id, ReturnType(Parameters...), Registry> :
    public detail::method_base<Registry> {
    // Deliberately no default for Inline: giving it a default on only one of
    // this template's two forward declarations in this file (the other is
    // below, next to override_impl) is accepted by gcc but rejected by clang
    // ("too few template arguments"), so every use spells out all three
    // arguments explicitly instead.
    template<auto Function, typename FunctionType, bool Inline>
    struct override_aux;

    // Aliases used in implementation only. Everything extracted from template
    // arguments is capitalized like the arguments themselves.
    using RegistryType = Registry;
    using rtti = typename Registry::rtti;
    using DeclaredParameters = mp11::mp_list<Parameters...>;
    using CallParameters =
        boost::mp11::mp_transform<detail::remove_virtual_, DeclaredParameters>;
    using VirtualParameters =
        typename detail::virtual_types<DeclaredParameters>;
    using Signature = auto(Parameters...) -> ReturnType;
    using FunctionPointer = auto (*)(detail::remove_virtual_<Parameters>...)
        -> ReturnType;

  public:
    //! Method singleton
    //!
    //! The only instance of `method`. Its `operator()` is used to call
    //! the method.
    static method fn;

    //! Call the method
    //!
    //! Call the method with `args`. The types of the arguments are the same as
    //! the method `Parameters...`, stripped from any `virtual\_` decorators.
    //!
    //! @param args The arguments for the method call
    //!
    //! @par Errors
    //!
    //! If `Registry` contains an @ref error_handler policy, call its `error`
    //! function with an object of one of the following types:
    //!
    //! @li @ref no_overrider: No overrider is applicable.
    //! @li @ref ambiguous_call: More than one overrider is applicable, and
    //! none is more specialized than all the others.
    //!
    auto operator()(typename BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                        StripVirtualDecorator<Parameters>::type... args) const
        -> ReturnType;

    //! Check if a next most specialized overrider exists
    //!
    //! Return `true` if a next most specialized overrider after _Fn_ exists,
    //! and @ref next can be called without causing a @ref bad_call.
    //!
    //! @par Requirements
    //!
    //! `Fn` must be a function that is an overrider of the method.
    //!
    //! @tparam Fn A function that is an overrider of the method.
    //! @return `true` if a next most specialized overrider exists
    template<auto Fn>
    static bool has_next();

    //! The next most specialized overrider
    //!
    //! A pointer to the next most specialized overrider after `Fn`, i.e. the
    //! overrider that would be called for the same tuple of virtual arguments
    //! if `Fn` was not present. Set to `nullptr` if no such overrider exists.
    //! @par Requirements
    //!
    //! `Fn` must be a function that is an overrider of the method.
    //!
    //! @tparam Fn A function that is an overrider of the method.

    // 'next' does not need any special treatment for Windows DLLs, because it
    // may be called only from within the overrider, registered with a registrar
    // in the same module.
    template<auto Fn>
    static FunctionPointer next;

    //! Add overriders to method
    //!
    //! `override`, instantiated as a static object, adds one or more overriders
    //! to an open-method.
    //!
    //! @par Requirements
    //!
    //! `Fn` must be a function that fulfills the following requirements:
    //!
    //! @li Have the same number of formal parameters as the method.
    //!
    //! @li Each @c virtual_ptr<T> in the method's parameter list must have a
    //! corresponding @c virtual_ptr<U> parameter in the same position in the
    //! overrider's parameter list, such that @c U is the same as @c T, or has
    //! @c T as an accessible unambiguous base.
    //!
    //! @li Each @c virtual_<T> in the method's parameter list must have a
    //! corresponding @c U parameter in the same position in the overrider's
    //! parameter list, such that @c U is the same as @c T, or has @c T as an
    //! accessible unambiguous base.
    //!
    //! @li All other formal parameters must have the same type as the method's
    //! corresponding parameters.
    //!
    //! @li The return type of the overrider must be the same as the method's
    //! return type or, if it is a polymorphic type, covariant with the method's
    //! return type.
    //!
    //! @tparam Fn One or more functions to the overrider list
    template<auto... Fn>
    class override {
        boost::mp11::mp_apply<
            detail::tuple,
            boost::mp11::mp_unique<
                boost::mp11::mp_list<override_aux<Fn, decltype(Fn), false>...>>>
            impl;
    };

    // Like override, but marks every overrider_info registered here as
    // inline_ = true (see overrider_info in preamble.hpp), making it
    // eligible for cross-module dedup during augment_methods()
    // consolidation - used by BOOST_OPENMETHOD_INLINE_OVERRIDE. Only an
    // overrider defined `inline` can legally have an identical definition
    // appear in more than one translation unit/module, which is why plain
    // `override` (above) never sets this.
    //
    // inline_ must be baked in as the `Inline` non-type template parameter
    // of override_aux/override_impl (below), not passed as a runtime
    // constructor argument to this class: override_aux::impl is a static
    // template data member whose dynamic initialization is not guaranteed to
    // happen before this class's own constructor body runs (taking its
    // address, `(void)&impl;` in override_aux's constructor, does not force
    // immediate construction) - a runtime assignment made here could run
    // *before* override_impl's constructor, which would then overwrite it
    // via its implicit default-construction of the overrider_info base.
    // Setting it inside override_impl's own constructor, alongside method,
    // pf, etc., is the only point guaranteed to run exactly once, at the
    // right time.
    template<auto... Fn>
    class inline_override {
        boost::mp11::mp_apply<
            detail::tuple,
            boost::mp11::mp_unique<
                boost::mp11::mp_list<override_aux<Fn, decltype(Fn), true>...>>>
            impl;
    };

  private:
    static constexpr auto Arity = boost::mp11::mp_count_if<
        mp11::mp_list<Parameters...>, detail::is_virtual>::value;

    // sanity checks
    static_assert((
        detail::validate_method_parameter<Parameters, Registry>::value && ...));
    static_assert(Arity > 0, "method has no virtual parameters");

    type_id vp_type_ids[Arity];

    std::size_t slots_strides[2 * Arity - 1];
    // Slots followed by strides. No stride for first virtual argument.
    // For 1-method: the offset of the method in the method table, which
    // contains a pointer to a function.
    // For multi-methods: the offset of the first virtual argument in the
    // method table, which contains a pointer to the corresponding cell in
    // the dispatch table, followed by the offset of the second argument and
    // the stride in the second dimension, etc.

    void resolve_type_ids();

    template<typename MethodArg, typename ArgType>
    auto vptr(const ArgType& arg) const -> vptr_type;

    template<typename MethodArgList, typename ArgType, typename... MoreArgTypes>
    auto resolve_uni(const ArgType& arg, const MoreArgTypes&... more_args) const
        -> detail::word;

    template<typename MethodArgList, typename ArgType, typename... MoreArgTypes>
    auto resolve_multi_first(
        const ArgType& arg,
        const MoreArgTypes&... more_args) const -> detail::word;

    template<
        std::size_t VirtualArg, typename MethodArgList, typename ArgType,
        typename... MoreArgTypes>
    auto resolve_multi_next(
        vptr_type dispatch, const ArgType& arg,
        const MoreArgTypes&... more_args) const -> detail::word;

    template<typename... ArgType>
    FunctionPointer resolve(const ArgType&... args) const;

    template<auto, typename>
    struct thunk;

    template<auto, typename>
    struct thunk;

    method();
    method(const method&) = delete;
    method(method&&) = delete;
    ~method();

    void resolve(); // virtual if Registry contains has_deferred_static_rtti

    static BOOST_NORETURN auto fn_not_implemented(
        detail::remove_virtual_<Parameters>... args) -> ReturnType;
    static BOOST_NORETURN auto fn_ambiguous(
        detail::remove_virtual_<Parameters>... args) -> ReturnType;

    template<
        auto Overrider, typename OverriderReturn,
        typename... OverriderParameters>
    struct thunk<Overrider, OverriderReturn (*)(OverriderParameters...)> {
        static auto fn(detail::remove_virtual_<Parameters>... arg)
            -> ReturnType;
        using OverriderVirtualParameters = detail::overrider_virtual_types<
            DeclaredParameters, mp11::mp_list<OverriderParameters...>,
            Registry>;
    };

    template<auto Function, typename FnReturnType, bool Inline = false>
    struct override_impl :
        std::conditional_t<
            Registry::has_deferred_static_rtti, detail::deferred_overrider_info,
            detail::overrider_info> {
        explicit override_impl(FunctionPointer* next = nullptr);
        void resolve_type_ids();

        static type_id vp_type_ids[Arity];
    };

    template<auto Function, typename FunctionType, bool Inline>
    struct override_aux;

    template<
        auto Function, typename FnReturnType, typename... FnParameters,
        bool Inline>
    struct override_aux<Function, FnReturnType (*)(FnParameters...), Inline> {
        override_aux() {
            (void)&impl;
        }

        static override_impl<Function, FnReturnType, Inline> impl;
    };
};

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<auto Fn>
typename method<Id, ReturnType(Parameters...), Registry>::FunctionPointer
    method<Id, ReturnType(Parameters...), Registry>::next;

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
method<Id, ReturnType(Parameters...), Registry>
    method<Id, ReturnType(Parameters...), Registry>::fn;

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<auto Function, typename FnReturnType, bool Inline>
type_id method<Id, ReturnType(Parameters...), Registry>::override_impl<
    Function, FnReturnType, Inline>::vp_type_ids[Arity];

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<
    auto Function, typename FnReturnType, typename... FnParameters, bool Inline>
typename method<Id, ReturnType(Parameters...), Registry>::
    template override_impl<Function, FnReturnType, Inline>
        method<Id, ReturnType(Parameters...), Registry>::override_aux<
            Function, FnReturnType (*)(FnParameters...), Inline>::impl;

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
method<Id, ReturnType(Parameters...), Registry>::method() {
    using namespace policies;

    this->slots_strides_ptr = slots_strides;

    if constexpr (!Registry::has_deferred_static_rtti) {
        resolve_type_ids();
    }

    this->vp_begin = vp_type_ids;
    this->vp_end = vp_type_ids + Arity;
    this->not_implemented = reinterpret_cast<void (*)()>(fn_not_implemented);
    this->ambiguous = reinterpret_cast<void (*)()>(fn_ambiguous);

    // zero-initalized static variable
    // coverity[uninit_use]
    Registry::static_::st.methods.push_back(*this);
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
void method<Id, ReturnType(Parameters...), Registry>::resolve_type_ids() {
    using namespace detail;
    this->method_type_id = rtti::template static_type<method>();
    this->return_type_id =
        rtti::template static_type<virtual_type<ReturnType, Registry>>();
    init_type_ids<
        Registry,
        mp11::mp_transform_q<
            mp11::mp_bind_back<virtual_type, Registry>,
            VirtualParameters>>::fn(this->vp_type_ids);
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
method<Id, ReturnType(Parameters...), Registry>::~method() {
    Registry::static_::st.methods.remove(*this);
}

// -----------------------------------------------------------------------------
// method dispatch

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
BOOST_FORCEINLINE auto
method<Id, ReturnType(Parameters...), Registry>::operator()(
    typename BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
        StripVirtualDecorator<Parameters>::type... args) const -> ReturnType {
    using namespace detail;
    auto pf = resolve(args...);

    return pf(std::forward<typename StripVirtualDecorator<Parameters>::type>(
        args)...);
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<typename... ArgType>
BOOST_FORCEINLINE
    typename method<Id, ReturnType(Parameters...), Registry>::FunctionPointer
    method<Id, ReturnType(Parameters...), Registry>::resolve(
        const ArgType&... args) const {
    using namespace detail;

    Registry::require_initialized();

    void (*pf)();

    if constexpr (Arity == 1) {
        pf = resolve_uni<mp11::mp_list<Parameters...>, ArgType...>(args...).pf;
    } else {
        pf = resolve_multi_first<mp11::mp_list<Parameters...>, ArgType...>(
                 args...)
                 .pf;
    }

    return reinterpret_cast<FunctionPointer>(pf);
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<typename MethodArg, typename ArgType>
BOOST_FORCEINLINE auto method<Id, ReturnType(Parameters...), Registry>::vptr(
    const ArgType& arg) const -> vptr_type {
    if constexpr (detail::is_virtual_ptr<ArgType>) {
        return arg.vptr();
    } else {
        decltype(auto) obj = virtual_traits<MethodArg, Registry>::peek(arg);

        if constexpr (detail::has_vptr_fn<decltype(obj), Registry>) {
            return boost_openmethod_vptr(obj, static_cast<Registry*>(nullptr));
        } else if constexpr (detail::has_vptr<
                                 virtual_traits<MethodArg, Registry>,
                                 decltype(obj)>) {
            return virtual_traits<MethodArg, Registry>::vptr(obj);
        } else {
            return Registry::template policy<policies::vptr>::dynamic_vptr(obj);
        }
    }
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<typename MethodArgList, typename ArgType, typename... MoreArgTypes>
BOOST_FORCEINLINE auto
method<Id, ReturnType(Parameters...), Registry>::resolve_uni(
    const ArgType& arg,
    const MoreArgTypes&... more_args) const -> detail::word {

    using namespace detail;
    using namespace policies;
    using namespace boost::mp11;

    if constexpr (is_virtual<mp_first<MethodArgList>>::value) {
        vptr_type vtbl = vptr<remove_virtual_<mp_first<MethodArgList>>>(arg);
        return vtbl[this->slots_strides[0]];
    } else {
        return resolve_uni<mp_rest<MethodArgList>>(more_args...);
    }
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<typename MethodArgList, typename ArgType, typename... MoreArgTypes>
BOOST_FORCEINLINE auto
method<Id, ReturnType(Parameters...), Registry>::resolve_multi_first(
    const ArgType& arg,
    const MoreArgTypes&... more_args) const -> detail::word {

    using namespace detail;
    using namespace boost::mp11;

    if constexpr (is_virtual<mp_first<MethodArgList>>::value) {
        vptr_type vtbl = vptr<remove_virtual_<mp_first<MethodArgList>>>(arg);
        std::size_t slot = this->slots_strides[0];

        // The first virtual parameter is special.  Since its stride is
        // 1, there is no need to store it. Also, the method table
        // contains a pointer into the multi-dimensional dispatch table,
        // already resolved to the appropriate group.
        auto dispatch = vtbl[slot].pw;
        return resolve_multi_next<1, mp_rest<MethodArgList>, MoreArgTypes...>(
            dispatch, more_args...);
    } else {
        return resolve_multi_first<mp_rest<MethodArgList>, MoreArgTypes...>(
            more_args...);
    }
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<
    std::size_t VirtualArg, typename MethodArgList, typename ArgType,
    typename... MoreArgTypes>
BOOST_FORCEINLINE auto
method<Id, ReturnType(Parameters...), Registry>::resolve_multi_next(
    vptr_type dispatch, const ArgType& arg,
    const MoreArgTypes&... more_args) const -> detail::word {

    using namespace detail;
    using namespace boost::mp11;

    if constexpr (is_virtual<mp_first<MethodArgList>>::value) {
        vptr_type vtbl = vptr<remove_virtual_<mp_first<MethodArgList>>>(arg);
        std::size_t slot = this->slots_strides[VirtualArg];
        std::size_t stride = this->slots_strides[Arity + VirtualArg - 1];
        dispatch = dispatch + vtbl[slot].i * stride;
    }

    if constexpr (VirtualArg + 1 == Arity) {
        return *dispatch;
    } else {
        return resolve_multi_next<
            VirtualArg + 1, mp_rest<MethodArgList>, MoreArgTypes...>(
            dispatch, more_args...);
    }
}

// -----------------------------------------------------------------------------
// Error handling

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<auto Fn>
inline auto method<Id, ReturnType(Parameters...), Registry>::has_next()
    -> bool {
    if (next<Fn> == fn_not_implemented) {
        return false;
    }

    if (next<Fn> == fn_ambiguous) {
        return false;
    }

    return true;
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
BOOST_NORETURN auto
method<Id, ReturnType(Parameters...), Registry>::fn_not_implemented(
    detail::remove_virtual_<Parameters>... args) -> ReturnType {
    using namespace policies;

    if constexpr (Registry::has_error_handler) {
        no_overrider error;
        detail::init_bad_call<method, rtti, 0u>::fn(
            error,
            detail::parameter_traits<Parameters, Registry>::peek(args)...);
        Registry::error_handler::error(error);
    }

    abort(); // in case user handler "forgets" to abort
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
BOOST_NORETURN auto
method<Id, ReturnType(Parameters...), Registry>::fn_ambiguous(
    detail::remove_virtual_<Parameters>... args) -> ReturnType {
    using namespace policies;

    if constexpr (Registry::has_error_handler) {
        ambiguous_call error;
        detail::init_bad_call<method, rtti, 0u>::fn(
            error,
            detail::parameter_traits<Parameters, Registry>::peek(args)...);
        Registry::error_handler::error(error);
    }

    abort(); // in case user handler "forgets" to abort
}

// -----------------------------------------------------------------------------
// overriders

namespace detail {

template<typename T, typename U>
struct same_reference_category {
    static constexpr bool value = (std::is_lvalue_reference<T>::value ==
                                   std::is_lvalue_reference<U>::value) &&
        (std::is_rvalue_reference<T>::value ==
         std::is_rvalue_reference<U>::value);
};
template<class T1, class T2, typename = void>
struct validate_overrider_parameter : std::false_type {
    static_assert(
        false_t<T1, T2>, "non-virtual parameter types must match exactly");
};

template<class T1, class T2>
struct validate_overrider_parameter<
    T1, T2,
    std::enable_if_t<
        is_virtual_ptr<T1> && is_virtual_ptr<T2> &&
        !same_reference_category<T1, T2>::value>> : std::false_type {
    static_assert(
        false_t<T1, T2>, "different virtual_ptr<> reference categories");
};

template<class T1, class T2>
struct validate_overrider_parameter<
    T1, T2, std::enable_if_t<is_virtual_ptr<T1> && !is_virtual_ptr<T2>>> :
    std::false_type {
    static_assert(
        false_t<T1, T2>,
        "virtual_ptr<> is required in overrider in same position as in "
        "method");
};

template<class T>
struct validate_overrider_parameter<T, T, void> : std::true_type {};

template<class T1, class T2>
struct validate_overrider_parameter<virtual_<T1>, T2, void> : std::true_type {};

template<class T1, class T2>
struct validate_overrider_parameter<virtual_<T1>, virtual_<T2>, void> :
    std::false_type {
    static_assert(false_t<T1, T2>, "virtual_<> is not allowed in overriders");
};

template<class T, class R>
struct validate_overrider_parameter<
    virtual_ptr<T, R>, virtual_ptr<T, R>, void> : std::true_type {};

template<class T1, class R, class T2, class R2>
struct validate_overrider_parameter<
    virtual_ptr<T1, R>, virtual_ptr<T2, R2>, void> : std::true_type {
    static_assert(std::is_same_v<R, R2>, "registry mismatch");
    using C1 = virtual_type<virtual_ptr<T1, R>, R>;
    using C2 = virtual_type<virtual_ptr<T2, R>, R>;
    static_assert(
        std::is_base_of_v<C1, C2> &&
            std::is_convertible_v<virtual_ptr<T2, R>, virtual_ptr<T1, R>>,
        "method parameter must be an unambiguous accessible base "
        "of corresponding overrider parameter");
};

template<class T1, class R, class T2, class R2>
struct validate_overrider_parameter<
    const virtual_ptr<T1, R>&, const virtual_ptr<T2, R2>&, void> :
    std::true_type {
    static_assert(std::is_same_v<R, R2>, "registry mismatch");
    using C1 = virtual_type<const virtual_ptr<T1, R>&, R>;
    using C2 = virtual_type<const virtual_ptr<T2, R>&, R>;
    static_assert(
        std::is_base_of_v<C1, C2> &&
            std::is_convertible_v<virtual_ptr<T2, R>, virtual_ptr<T1, R>>,
        "method parameter must be an unambiguous accessible base "
        "of corresponding overrider parameter");
};

template<class T1, class R, class T2, class R2>
struct validate_overrider_parameter<
    virtual_ptr<T1, R>&&, virtual_ptr<T2, R2>&&, void> : std::true_type {
    static_assert(std::is_same_v<R, R2>, "registry mismatch");
    using C1 = virtual_type<virtual_ptr<T1, R>&&, R>;
    using C2 = virtual_type<virtual_ptr<T2, R>&&, R>;
    static_assert(
        std::is_base_of_v<C1, C2> &&
            std::is_convertible_v<virtual_ptr<T2, R>, virtual_ptr<T1, R>>,
        "method parameter must be an unambiguous accessible base "
        "of corresponding overrider parameter");
};

} // namespace detail

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<
    auto Overrider, typename OverriderReturn, typename... OverriderParameters>
auto method<Id, ReturnType(Parameters...), Registry>::
    thunk<Overrider, OverriderReturn (*)(OverriderParameters...)>::fn(
        detail::remove_virtual_<Parameters>... arg) -> ReturnType {
    using namespace detail;
    static_assert(
        (validate_overrider_parameter<Parameters, OverriderParameters>::value &&
         ...),
        "virtual_ptr category mismatch");
    return Overrider(
        detail::parameter_traits<Parameters, Registry>::template cast<
            OverriderParameters>(
            std::forward<detail::remove_virtual_<Parameters>>(arg))...);
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<auto Function, typename FnReturnType, bool Inline>
method<Id, ReturnType(Parameters...), Registry>::override_impl<
    Function, FnReturnType, Inline>::override_impl(FunctionPointer* p_next) {
    using namespace detail;

    // static variable this->method below is zero-initialized but gcc and clang
    // don't always see that.

#ifdef BOOST_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
#endif

#ifdef BOOST_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

    // zero-initalized static variable
    // coverity[uninit_use]
    if (overrider_info::method) {
        BOOST_ASSERT(overrider_info::method == &method::fn);
        return;
    }

#ifdef BOOST_CLANG
#pragma clang diagnostic pop
#endif

#ifdef BOOST_GCC
#pragma GCC diagnostic pop
#endif

    overrider_info::method = &method::fn;
    // Baked in as a template parameter (not a runtime constructor argument):
    // this static object's dynamic initialization is not guaranteed to
    // happen at any particular point relative to other code (see class
    // override/inline_override in this file), so Inline must be known here,
    // at the one place this object's fields are set exactly once.
    overrider_info::inline_ = Inline;

    if constexpr (!Registry::has_deferred_static_rtti) {
        resolve_type_ids();
    }

    this->next = reinterpret_cast<void (**)()>(
        p_next ? p_next : &method::next<Function>);

    using Thunk = thunk<Function, decltype(Function)>;
    this->pf = reinterpret_cast<void (*)()>(Thunk::fn);

    this->vp_begin = vp_type_ids;
    this->vp_end = vp_type_ids + Arity;

    method::fn.overriders.push_back(*this);
}

template<
    typename Id, typename... Parameters, typename ReturnType, class Registry>
template<auto Function, typename FnReturnType, bool Inline>
void method<Id, ReturnType(Parameters...), Registry>::override_impl<
    Function, FnReturnType, Inline>::resolve_type_ids() {
    using namespace detail;

    this->return_type = Registry::rtti::template static_type<
        virtual_type<FnReturnType, Registry>>();
    this->type = Registry::rtti::template static_type<decltype(Function)>();
    using Thunk = thunk<Function, decltype(Function)>;
    detail::
        init_type_ids<Registry, typename Thunk::OverriderVirtualParameters>::fn(
            this->vp_type_ids);
}

// =============================================================================
// register_classes

namespace detail {

#if BOOST_OPENMETHOD_HAS_REFLECTION

template<class Method>
struct method_traits_aux;

template<
    typename Id, typename ReturnType, typename... Parameters, class Registry>
struct method_traits_aux<method<Id, ReturnType(Parameters...), Registry>> {
    // The classes the method dispatches on. Not its return type: a covariant
    // return type is registered when the scan finds it deriving from one of
    // these, or when it is listed - as in C++17 - not as a root of its own.
    // A root return type would make `initialize` demand a registration for
    // the return type of every overrider, `std::ostringstream` included.
    using type = mp11::mp_transform_q<
        mp11::mp_bind_back<virtual_type, Registry>,
        virtual_types<mp11::mp_list<Parameters...>>>;
};

// Read from reflection, by `substitute`-ing a method into it and taking the
// template arguments of the result.
template<class Method>
using method_classes = typename method_traits_aux<Method>::type;

// How `register_classes` interprets an argument group, and the items in it.
enum class register_classes_kind : unsigned char {
    invalid,    // not a reflection register_classes accepts, or a mixed group
    empty,      // `{}` - carries no kind, and takes no part in the ordering
    namespaces, // namespaces to scan
    classes,    // classes to register
    registries, // registries to add the classes to
};

consteval auto is_registry_type(std::meta::info type) -> bool {
    return std::meta::extract<bool>(std::meta::substitute(
        ^^is_registry,
        {
            type}));
}

// How `register_classes` interprets one item of one of its argument groups.
consteval auto register_classes_item_kind(std::meta::info item)
    -> register_classes_kind {
    auto entity = std::meta::dealias(item);

    if (std::meta::is_namespace(entity)) {
        return register_classes_kind::namespaces;
    }

    if (std::meta::is_class_type(entity)) {
        // A registry is always complete where classes are registered in it, so
        // an incomplete class cannot be one.
        if (std::meta::is_complete_type(entity) && is_registry_type(entity)) {
            return register_classes_kind::registries;
        }

        return register_classes_kind::classes;
    }

    return register_classes_kind::invalid;
}

// True if the group holds an item that is not a reflection `register_classes`
// accepts. The two checks below let such a group pass, so a mistake yields one
// diagnostic instead of three.
template<auto Group>
consteval auto register_classes_group_has_unknown() -> bool {
    for (auto index = 0u; index != Group.size; ++index) {
        if (register_classes_item_kind(Group.items[index]) ==
            register_classes_kind::invalid) {
            return true;
        }
    }

    return false;
}

// The kind of the items in `Group`; `invalid` if they are not all of the same
// kind, or if one of them is not a reflection `register_classes` accepts.
template<auto Group>
consteval auto register_classes_group_kind() -> register_classes_kind {
    auto kind = register_classes_kind::empty;

    for (auto index = 0u; index != Group.size; ++index) {
        auto item = register_classes_item_kind(Group.items[index]);

        if (item == register_classes_kind::invalid) {
            return register_classes_kind::invalid;
        }

        if (kind != register_classes_kind::empty && item != kind) {
            return register_classes_kind::invalid;
        }

        kind = item;
    }

    return kind;
}

template<auto... Groups>
consteval auto register_classes_groups_are_valid() -> bool {
    return (... && !register_classes_group_has_unknown<Groups>());
}

template<auto... Groups>
consteval auto register_classes_groups_are_homogeneous() -> bool {
    return (
        ... &&
        (register_classes_group_has_unknown<Groups>() ||
         register_classes_group_kind<Groups>() !=
             register_classes_kind::invalid));
}

// The groups must come in the order the `register_classes_kind` enumerators
// are declared in: namespaces, classes, registries.
template<auto... Groups>
consteval auto register_classes_groups_are_ordered() -> bool {
    // `register_classes<>` registers the classes of a scan of the global
    // namespace, and is the shape `BOOST_OPENMETHOD_REGISTER_CLASSES()`
    // expands to. It must be taken before the array below is formed: with no
    // group, that array has size zero, which is not standard C++ - GCC rejects
    // it outright under -Wpedantic, clang under -pedantic-errors.
    if constexpr (sizeof...(Groups) == 0) {
        return true;
    } else {
        register_classes_kind kinds[] = {
            register_classes_group_kind<Groups>()...};
        auto last = register_classes_kind::empty;

        for (auto kind : kinds) {
            // An empty group carries no kind, and a group the checks above
            // rejected has no meaningful one either.
            if (kind == register_classes_kind::empty ||
                kind == register_classes_kind::invalid) {
                continue;
            }

            if (kind < last) {
                return false;
            }

            last = kind;
        }

        return true;
    }
}

// The items of every group whose kind is `Kind`, in order, without repeats.
template<auto... Groups>
consteval auto register_classes_items(register_classes_kind kind)
    -> std::vector<std::meta::info> {
    std::vector<std::meta::info> items;

    // `Groups` is empty for `register_classes<>`, which names nothing and
    // scans the global namespace. The fold below then expands to nothing and
    // never reads `kind`, which GCC reports under -Wunused-but-set-parameter.
    static_cast<void>(kind);

    (..., [&] {
        for (auto index = 0u; index != Groups.size; ++index) {
            auto item = Groups.items[index];

            if (register_classes_item_kind(item) == kind) {
                push_unique(items, std::meta::dealias(item));
            }
        }
    }());

    return items;
}

template<auto... Groups>
consteval auto register_classes_registries_info() -> std::meta::info {
    return std::meta::substitute(
        ^^mp11::mp_list,
        register_classes_items<Groups...>(register_classes_kind::registries));
}

// `mp_list<Registry...>` for the registries the groups name.
//
// clang-format off: the formatter predates P2996 and eats the spaces around
// the splice, leaving `typename[:...:]`.
template<auto... Groups>
using register_classes_registries =
    typename [: register_classes_registries_info<Groups...>() :];
// clang-format on

// True if every listed class is complete. An incomplete one - a forward
// declaration left in a group by mistake - would otherwise surface as an
// exception thrown from inside constant evaluation, which names no class.
template<auto... Groups>
consteval auto register_classes_classes_are_complete() -> bool {
    auto types =
        register_classes_items<Groups...>(register_classes_kind::classes);

    for (auto type : types) {
        if (!std::meta::is_complete_type(type)) {
            return false;
        }
    }

    return true;
}

// The scan for the argument groups `Groups`: the listed namespaces - or the
// global one - for the `method` specializations they name and the classes
// they declare.
template<auto... Groups>
consteval auto reflected_scan_of() -> reflected_scan {
    auto listed =
        register_classes_items<Groups...>(register_classes_kind::classes);

    for (auto& type : listed) {
        type = std::meta::remove_cv(type);
    }

    return scan_namespaces(
        register_classes_items<Groups...>(register_classes_kind::namespaces),
        listed, ^^method);
}

// Append to `entries` the record `use_class_aux` expects for `type` - the
// class, itself again as its own improper base, and every registered class
// above it - if `type` is registered at all: if it is one of `roots`, or
// reaches one through public inheritance. The classes above it that are
// registered are the ones that reach a root themselves, less the ones that
// repeated inheritance makes ambiguous, which `is_dispatchable_base` tells
// apart.
//
// Every registered ancestor is recorded, not only the direct bases.
// `initialize` derives the direct bases from the closure of what the records
// say, so a record that carries the whole ancestry stands on its own, whatever
// another registration - in another translation unit, over other namespaces -
// records alongside it. Keeping only the nearest ancestors would need the
// registered set to be the same everywhere, and it is not.
//
// An ancestor the scan does not declare - in a namespace it does not enter, a
// specialization of a class template, a local class - is given a record of its
// own here, recursively, as nothing else would. A class the scan declares, or
// finds through an alias, gets its record when its own turn comes. `extra`
// holds the ones done here, so that each is done once.
consteval void reflected_class_entries(
    std::meta::info type, const std::vector<std::meta::info>& roots,
    const reflected_scan& scan, std::vector<std::meta::info>& extra,
    std::vector<std::meta::info>& entries) {
    std::vector<base_record> bases;

    if (!walk_bases(type, roots, bases)) {
        return;
    }

    std::vector<std::meta::info> entry;
    entry.push_back(type);
    // The class as its own improper base, as `inheritance_map` does.
    // `initialize` discards it, and `use_class_aux` cannot hold an empty base
    // array.
    entry.push_back(type);

    for (std::size_t index = 1; index != bases.size(); ++index) {
        const auto& base = bases[index];

        if (!base.reaches_root) {
            continue;
        }

        if (base.repeated && !is_dispatchable_base(type, base.type)) {
            continue;
        }

        entry.push_back(base.type);

        if (!contains(roots, base.type) && !contains(scan.aliased, base.type) &&
            !scan_declares(base.type, scan.namespaces) &&
            !contains(extra, base.type)) {
            extra.push_back(base.type);
            reflected_class_entries(base.type, roots, scan, extra, entries);
        }
    }

    entries.push_back(std::meta::substitute(^^mp11::mp_list, entry));
}

// The classes to register for the argument groups `Groups`, in each of
// `Registries`, an `mp_list` of registries. Returns
// `mp_list<mp_list<Registry, mp_list<Class, Class, Base...>...>...>`: one list
// per registry, headed by the registry, holding the records `use_class_aux`
// expects. The scan runs once, for all the registries: only the roots depend on
// the registry - the classes the groups list, and the classes the registry's
// methods dispatch on. Registered are the roots, and every class the scan
// found, or was led to, that derives from one.
//
// The work is done here, in reflection, and not with `mp11` over the lists the
// scan produces. A scan of the global namespace reaches every class in the
// program that is not in `std` or `boost`, and instantiating a trait once per
// pair of them costs far more than walking their base classes does.
template<class Registries, auto... Groups>
consteval auto reflected_registered_classes_info() -> std::meta::info {
    auto scan = reflected_scan_of<Groups...>();
    auto registries = std::meta::template_arguments_of(^^Registries);
    std::vector<std::meta::info> lists;

    for (auto registry : registries) {
        auto roots = scan.listed;

        for (auto found : scan.methods) {
            // A method's third template argument is its registry.
            if (std::meta::template_arguments_of(found)[2] != registry) {
                continue;
            }

            auto list = std::meta::dealias(
                std::meta::substitute(
                    ^^method_classes,
                    {
                        found}));
            auto types = std::meta::template_arguments_of(list);

            for (auto type : types) {
                push_unique(roots, std::meta::remove_cv(type));
            }
        }

        std::vector<std::meta::info> entries, extra;
        entries.push_back(registry);

        for (auto root : roots) {
            // A method written by hand may be declared on a class that this
            // translation unit never completes. It is registered where it is.
            if (std::meta::is_complete_type(root)) {
                reflected_class_entries(root, roots, scan, extra, entries);
            }
        }

        for (auto type : scan.declared) {
            if (!contains(roots, type)) {
                reflected_class_entries(type, roots, scan, extra, entries);
            }
        }

        for (auto type : scan.aliased) {
            if (!contains(roots, type)) {
                reflected_class_entries(type, roots, scan, extra, entries);
            }
        }

        lists.push_back(std::meta::substitute(^^mp11::mp_list, entries));
    }

    return std::meta::substitute(^^mp11::mp_list, lists);
}

// `mp_list<mp_list<Registry, mp_list<Class, Class, Base...>...>...>`, ready
// for `use_reflected_classes`.
//
// clang-format off: the formatter predates P2996 and eats the spaces around the
// splice, leaving `typename[:...:]`.
template<class Registries, auto... Groups>
using reflected_registered_classes =
    typename [: reflected_registered_classes_info<Registries, Groups...>() :];
// clang-format on

#endif

} // namespace detail

#if BOOST_OPENMETHOD_HAS_REFLECTION

// MrDocs' front-end does not implement P2996, so it never sees this branch.
// The reference documentation for `register_classes` is on the stub in the
// `#elif` branch below; keep the two in step.

//! @see @ref register_classes for documentation.
template<detail::reflection_group... Groups>
class register_classes {
    static_assert(
        detail::register_classes_groups_are_valid<Groups...>(),
        "a group holds reflections of namespaces, classes or registries");
    static_assert(
        detail::register_classes_groups_are_homogeneous<Groups...>(),
        "a group holds one kind of reflection; put the namespaces, the "
        "classes and the registries in groups of their own");
    static_assert(
        detail::register_classes_groups_are_ordered<Groups...>(),
        "order the groups as namespaces, classes, registries");
    static_assert(
        detail::register_classes_classes_are_complete<Groups...>(),
        "a listed class must be complete");

    using found_registries = detail::register_classes_registries<Groups...>;
    using registries = mp11::mp_if<
        mp11::mp_empty<found_registries>,
        mp11::mp_list<BOOST_OPENMETHOD_DEFAULT_REGISTRY>, found_registries>;

    template<class... Lists>
    static auto use(mp11::mp_list<Lists...>*) -> void {
        (..., detail::use_reflected_classes(static_cast<Lists*>(nullptr)));
    }

  public:
    register_classes() {
        use(static_cast<
            detail::reflected_registered_classes<registries, Groups...>*>(
            nullptr));
    }
};

#elif defined(__MRDOCS__)

// Documentation stub. MrDocs compiles this branch instead of the real
// declaration above, which its front-end cannot parse. It names no `std::meta`
// type, so nothing has to stand in for one.

//! Find the classes taking part in dispatch by reflection, and register them
//! (C++26 and above).
//!
//! @warning Requires C++26 reflection (P2996): a compiler that implements
//! it, and the flag that turns it on - `-freflection` with GCC 16. Without it
//! this class template does not exist: `BOOST_OPENMETHOD_HAS_REFLECTION` is 0,
//! and classes are registered with @ref use_classes or
//! @ref BOOST_OPENMETHOD_CLASSES. @ref BOOST_OPENMETHOD_REGISTER_CLASSES wraps
//! this class in a macro that expands to a no-op under C++17.
//!
//! `register_classes` is a registrar class that finds the classes taking part
//! in dispatch by reflection, and adds them to one or more registries. It makes
//! @ref use_classes unnecessary in most cases.
//!
//! The arguments are groups of reflections, each written in braces - or, for a
//! group of one, as the reflection itself. A group holds one kind of
//! reflection, and the groups come in this order; each one is optional, and
//! omitting all of them is the common case:
//!
//! @li **Namespaces** to scan: `{^^app, ^^zoo}`, `{\^^::}`.
//! @li **Classes** to register: `{^^Animal}`. They are registered whether a
//! method dispatches on them or not, and they are extra roots for the scan,
//! which registers the classes it finds deriving from them. A listed class
//! must be complete.
//! @li **Registries** to register the classes in: `{^^my_registry}`. Each one
//! receives the registration. The default is
//! `BOOST_OPENMETHOD_DEFAULT_REGISTRY`.
//!
//! @code
//! register_classes<^^app, ^^my_registry>
//! register_classes<{^^app, ^^zoo}, {^^r1, ^^r2}>
//! register_classes<{^^Animal, ^^Dog}>
//! register_classes<^^my_registry>
//! register_classes<>
//! @endcode
//!
//! A group holding two kinds of reflection is an error, and so is a group out
//! of order. Braces around a single reflection change nothing:
//! `register_classes<^^app>` and `register_classes<{^^app}>` are the same
//! registration.
//!
//! The scan covers the listed namespaces, the namespaces nested in them, and
//! the classes nested in the classes it declares, except `std` and `boost`:
//! walking those would cost a great deal and find nothing, as a method cannot
//! be declared on a class the program has never heard of. The exclusion applies
//! to recursion only, so a class in `std` or `boost` is registered by *listing*
//! the namespace it is in. The scan never goes through an alias: a namespace
//! alias is not entered, and a type alias registers the class it names but is
//! not walked into. The scan finds the methods of each target registry and
//! collects the classes they dispatch on; those, and the listed classes, are
//! the *roots*. It registers the roots, and every class it finds that derives
//! from one of them. A base class that no method dispatches on, and that is not
//! listed, is not registered: it could never be selected on.
//!
//! A class between a registered class and a root is registered too, wherever
//! it is declared: in a namespace the scan does not enter, or as a
//! specialization of a class template, which the scan cannot find on its own.
//! The namespaces decide where the *derived* classes are looked for, not which
//! bases are recorded; and each class is recorded with every registered class
//! above it, so that a registration stands on its own, whatever another one,
//! in another translation unit, registers alongside it.
//!
//! If no namespace is given, the global namespace is scanned - whatever the
//! other groups hold, and for @ref BOOST_OPENMETHOD_REGISTER_CLASSES too. Name
//! a namespace to scan only that one.
//!
//! The scan runs when the registrar is instantiated, which current compilers
//! do at the end of the translation unit: it sees the whole file, wherever the
//! registrar sits. The standard promises less. A class or a method the scan
//! would select, declared after the registrar in the same translation unit,
//! makes the program ill-formed, no diagnostic required. Put the registrar
//! **after** the declarations it is meant to find, at the bottom of the file.
//!
//! A method is found through any namespace member that names its `method`
//! specialization: the alias @ref BOOST_OPENMETHOD declares for it, a `using`
//! declaration written by hand, or any of its registrar objects - the object
//! @ref BOOST_OPENMETHOD_OVERRIDE creates, or one written by hand. None of
//! those requires the method to have an overrider. A core interface method
//! whose `method<...>` type is spelled out in full at every use, with neither a
//! `using` declaration nor an overrider, is named by nothing and is not found;
//! its classes must be registered with @ref use_classes. A method's return
//! type is not a root: a covariant return type is registered like any other
//! class, when it derives from a root, or when it is listed.
//!
//! Virtual and multiple inheritance are supported. Unlike @ref use_classes,
//! which rejects it, repeated inheritance is not an error here: a class is
//! recorded under the bases it reaches unambiguously through public
//! inheritance, and a base it inherits more than once, which no reference to
//! it can be converted to, is left out of its list. A class with no registered
//! class above it is not registered at all.
//!
//! @tparam Groups Braced groups of reflections: namespaces, classes,
//! registries, in that order.
//!
//! @see [Core API](xref:ROOT:core_api.adoc)
template<auto... Groups>
class register_classes {
  public:
    //! Register the selected classes in each target registry.
    register_classes();
};

#endif

//! Aliases for the most frequently used types in the library.
namespace aliases {

using boost::openmethod::final_virtual_ptr;
using boost::openmethod::virtual_;
using boost::openmethod::virtual_ptr;

} // namespace aliases

// ==============================================================================
// Exposition only

#ifdef __MRDOCS__

//! Blueprint for a specialization of @ref virtual_traits (exposition only).
//!
//! Specializations of @ref virtual_traits must implement the members listed
//! here.
//!
//! @tparam T The type of a virtual parameter of a method.
//! @tparam Registry A @ref registry.
template<typename T, class Registry>
struct VirtualTraits {
    //! Class to use for dispatch.
    //!
    //! Aliases to the class to be considered during method dispatch to determine
    //! which overrider to select, and which type_id to use for error reporting.
    //! `virtual_traits<T>::virtual_type` aliases to `Class` if `T` is `Class&`,
    //! `const Class&`, `Class*`, `const Class*`, `virtual_ptr<Class>`,
    //! `virtual_ptr<const Class>`, `std::shared_ptr<Class>`,
    //! `std::shared_ptr<const Class>`, `virtual_ptr<std::shared_ptr<Class>>`,
    //! etc.
    //!
    //! @par Requirements
    //!
    //! `virtual_type` must be an alias to an *unadorned* *class* type, *not*
    //! cv-qualified.
    using virtual_type = detail::unspecified;

    //! Returns a reference to the object to use for dispatch.
    //!
    //! Return a reference to the object to use for dispatch. `arg` may not be
    //! copied, moved or altered in any way.
    //!
    //! @param arg An argument passed to the method call.
    //! @return A reference to an object.
    static auto peek(T arg) -> const virtual_type&;

    // Added by the `std::any` interop, under the name `type_vptr`. An `any`
    // dispatches on the type of the value it contains, which the rtti policy
    // cannot see: `dynamic_type` on the `any` itself yields the wrapper.

    //! Returns a *reference* to the v-table pointer for an object.
    //!
    //! `vptr` is optional. It is called on the object returned by @ref peek,
    //! not on the method argument itself. A method acquires the v-table
    //! pointer of a virtual argument from the first of the following that is
    //! available: a `boost_openmethod_vptr` function, found by ADL on the
    //! peeked object; `vptr`; @ref policies::VptrFn::dynamic_vptr of the
    //! registry's @ref policies::vptr policy.
    //!
    //! Implement `vptr` only if the v-table pointer cannot be obtained from
    //! the dynamic type of the peeked object, as reported by the registry's
    //! @ref policies::rtti policy, or if it is already at hand. The former is
    //! the case for `any`-like types: their dynamic type is the wrapper, not
    //! the value they contain. The `std::any` specializations read the
    //! @ref type_id of the contained value from `arg.type()`, and pass it to
    //! @ref policies::VptrFn::vptr. The latter is the case for a wide type
    //! that caches the v-table pointer: @ref virtual_any returns the one it
    //! acquired when it was created, without a lookup.
    //!
    //! `vptr` must return a *reference*, not a value, so that the caller
    //! observes the current v-table pointer if the registry contains the
    //! @ref policies::indirect_vptr policy and `initialize` is called again.
    //!
    //! @param arg The object returned by @ref peek.
    //! @return A reference to the v-table pointer for `arg`.
    static auto vptr(const virtual_type& arg) -> const vptr_type&;

    //! Casts a virtual argument.
    //!
    //! `cast` is responsible for passing virtual arguments from method to
    //! overrider. In general, this requires some form of adjustment, because a
    //! virtual parameter in the overrider usually has a different type than the
    //! corresponding parameter in the method. Typically, the adjustment
    //! consists of a cast, performed via `static_cast`, `dynamic_cast`, or
    //! other means, depending on the type of the argument and the rtti policy
    //! of the method. `cast` may return the adjusted argument by reference or
    //! as a temporary value.
    //!
    //! @tparam T The type of the virtual parameter in the method.
    //! @tparam U The type of the virtual parameter in the overrider.
    //! @param arg The argument passed to the method call.
    //! @return A value that can be passed as a U.
    template<typename U>
    static auto cast(T arg) -> detail::unspecified;

    //! Rebind to a another class (smart pointers only).
    //!
    //! If `T` is a smart pointer, `rebind<U>` is the same kind of smart
    //! pointer, but pointing to a `U`.
    //!
    //! @note `rebind` must be implemented @em only for smart pointer classes
    //! that can be used as object pointers by @ref virtual_ptr in place of
    //! plain pointers.
    //!
    //! @tparam U The new element type.
    template<class U>
    using rebind = detail::unspecified;
};

#endif

} // namespace boost::openmethod

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
