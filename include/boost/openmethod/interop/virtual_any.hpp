// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_INTEROP_VIRTUAL_ANY_HPP
#define BOOST_OPENMETHOD_INTEROP_VIRTUAL_ANY_HPP

#include <boost/openmethod/core.hpp>

#include <type_traits>
#include <utility>

namespace boost::openmethod {

template<class Any, class Registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY>
class virtual_any;

BOOST_OPENMETHOD_OPEN_NAMESPACE_DETAIL_UNLESS_MRDOCS

//! Test if argument is a wide `any` (exposition only)
//!
//! Evaluates to `true` if `T` is a specialization of @ref virtual_any, and
//! `false` otherwise.
//!
//! This constrains the constructor and the assignment operator of
//! @ref virtual_any that take a value, excluding every specialization of the
//! wide type, not only the one matching this `virtual_any`. A matching
//! argument then selects the copy or move operation instead of being stored
//! inside the `any`; any other specialization is rejected outright rather
//! than stored: a `virtual_any` is not a registered class, so its
//! @ref registry::static_vptr would be null.
//!
//! @tparam T A type.
template<typename T>
constexpr bool IsVirtualAny = false;

//! Recognize a virtual_any (exposition only)
//!
//! The specialization of @ref IsVirtualAny that matches a
//! `virtual_any`, and evaluates to `true`.
//!
//! @tparam Any An `any` type.
//! @tparam Registry A @ref registry.
template<class Any, class Registry>
constexpr bool IsVirtualAny<virtual_any<Any, Registry>> = true;

BOOST_OPENMETHOD_CLOSE_NAMESPACE_DETAIL_UNLESS_MRDOCS

namespace detail {

// Registers Class under the `any` root class, plus the root itself.
// odr-used by the any interop's virtual_traits - cast (one instantiation
// per overrider parameter) and vptr (root only), on both the plain `any`
// and the virtual_any specializations - and by virtual_any's value
// constructor, value assignment and emplace. Naming a type as an overrider
// parameter, or storing a value in a virtual_any, thus registers it
// automatically; dispatching on a virtual_any registers the root, because
// that goes through vptr. mp_unique collapses the Class == Root case to the
// root entry alone.
template<class Registry, class Root, class Class = Root>
inline boost::mp11::mp_apply<
    tuple,
    boost::mp11::mp_transform_q<
        boost::mp11::mp_bind_front<use_class_aux, Registry>,
        boost::mp11::mp_unique<boost::mp11::mp_list<
            boost::mp11::mp_list<Root, Root>,
            boost::mp11::mp_list<Class, Root>>>>>
    use_any_classes;

} // namespace detail

//! A wide `any`, combining an `any` and a pointer to a v-table.
//!
//! `virtual_any` is to `any` what @ref virtual_ptr is to a pointer: it
//! carries the v-table pointer for the value stored in the `any`, so
//! methods dispatch on the contained type without looking it up on every
//! call. Unlike `virtual_ptr`, it *owns* its object: the `any` is held by
//! value.
//!
//! The v-table pointer is acquired when the `virtual_any` is created:
//! either from the dynamic type of an existing `any` (a hash table
//! lookup, via `virtual_traits<const Any&, Registry>::vptr`), or
//! statically, when the contained type is known at compile time (the
//! value constructor and @ref emplace use @ref registry::static_vptr).
//!
//! Contained types are registered automatically, as classes derived from
//! `Any`: naming a type as the parameter of an overrider - or storing a
//! value in a `virtual_any` - registers it in `Registry`.
//!
//! Methods take `virtual_any` parameters by reference:
//! `const virtual_any&`, `virtual_any&` or `virtual_any&&`. Overriders receive
//! the *contained* type, by a reference of a compatible category - or the
//! `virtual_any` itself, unchanged, for a catch-all overrider.
//!
//! The contained value cannot be replaced through a `virtual_any` other
//! than via assignment or @ref emplace, which re-derive the v-table
//! pointer, thus maintaining the invariant that the v-table pointer
//! corresponds to the contained type.
//!
//! `Any` can be `std::any`, `boost::any`, or any type that has an
//! `any`-like interface, and specializes `virtual_traits` for its
//! reference types, providing `vptr` and `cast`.
//!
//! @tparam Any An `any` type.
//! @tparam Registry A @ref registry.
//!
//! @par Example
//! include:virtual_any.cpp#classes;method;dispatch
//!
//! @see [Interoperation with `any`](xref:ROOT:interop_any.adoc)
template<class Any, class Registry>
class virtual_any {
    static constexpr bool use_indirect_vptrs = Registry::has_indirect_vptr;

    Any obj;
    std::conditional_t<use_indirect_vptrs, const vptr_type*, vptr_type> vp;

    template<typename, class>
    friend struct virtual_traits;

    // The v-table pointer, by reference, for virtual_traits::vptr, which
    // must return one so that the caller observes a re-initialize. An
    // indirect registry stores the address of the cell holding it; a direct
    // one stores it outright, in a member that outlives the call.
    auto vptr_ref() const -> const vptr_type& {
        if constexpr (use_indirect_vptrs) {
            return *vp;
        } else {
            return vp;
        }
    }

  public:
    //! Construct an empty `virtual_any`.
    //!
    //! The `any` is empty, and the v-table pointer is null.
    virtual_any() :
        obj(), vp(detail::box_vptr<use_indirect_vptrs>(detail::null_vptr)) {
    }

    //! Construct from an `any` (copy).
    //!
    //! Copies `other`, and acquires the v-table pointer for the contained
    //! value, using `virtual_traits<const Any&, Registry>::vptr`.
    //!
    //! @param other An `any`.
    //!
    //! @par Example
    //! include:virtual_any.cpp#from_any
    virtual_any(const Any& other) :
        obj(other), vp(detail::box_vptr<use_indirect_vptrs>(
                        detail::acquire_vptr<Registry>(obj))) {
    }

    //! Construct from an `any` (move).
    //!
    //! Moves `other`, and acquires the v-table pointer for the contained
    //! value, using `virtual_traits<const Any&, Registry>::vptr`.
    //!
    //! @param other An `any`.
    virtual_any(Any&& other) :
        obj(std::move(other)), vp(detail::box_vptr<use_indirect_vptrs>(
                                   detail::acquire_vptr<Registry>(obj))) {
    }

    //! Construct from a value.
    //!
    //! Stores `value` in the `any`, and sets the v-table pointer to the
    //! @ref registry::static_vptr for its type - no hash table lookup is
    //! involved. The type of `value`, stripped from reference and
    //! cv-qualifiers, is registered automatically in `Registry`, as a
    //! class derived from `Any`.
    //!
    //! @tparam T The type of the value.
    //! @param value The value to store.
    //!
    //! @par Example
    //! include:virtual_any.cpp#from_value
    template<
        typename T,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                IsVirtualAny<std::decay_t<T>> == false>,
        typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<T>, Any> &&
            std::is_constructible_v<Any, T&&>>>
    virtual_any(T&& value) :
        obj(std::forward<T>(value)),
        vp(detail::box_vptr<use_indirect_vptrs>(
            Registry::template static_vptr<std::decay_t<T>>)) {
        using namespace detail;

        (void)&use_any_classes<Registry, Any, std::decay_t<T>>;
        Registry::require_initialized();
        BOOST_ASSERT(unbox_vptr(vp) != nullptr);
    }

    //! Copy constructor.
    virtual_any(const virtual_any& other) = default;

    //! Move constructor.
    //!
    //! Moves the `any`, and sets `other`'s v-table pointer to null.
    //!
    //! @param other A `virtual_any`.
    virtual_any(virtual_any&& other) : obj(std::move(other.obj)), vp(other.vp) {
        using namespace detail;

        other.vp = box_vptr<use_indirect_vptrs>(null_vptr);
    }

    //! Copy assignment operator.
    auto operator=(const virtual_any& other) -> virtual_any& = default;

    //! Move assignment operator.
    //!
    //! Moves the `any`, and sets `other`'s v-table pointer to null.
    //!
    //! @param other A `virtual_any`.
    auto operator=(virtual_any&& other) -> virtual_any& {
        using namespace detail;

        obj = std::move(other.obj);
        vp = other.vp;
        other.vp = box_vptr<use_indirect_vptrs>(null_vptr);

        return *this;
    }

    //! Assign from an `any` (copy).
    //!
    //! Copies `other`, and re-acquires the v-table pointer for the
    //! contained value.
    //!
    //! @param other An `any`.
    auto operator=(const Any& other) -> virtual_any& {
        using namespace detail;

        obj = other;
        vp = box_vptr<use_indirect_vptrs>(acquire_vptr<Registry>(obj));

        return *this;
    }

    //! Assign from an `any` (move).
    //!
    //! Moves `other`, and re-acquires the v-table pointer for the
    //! contained value.
    //!
    //! @param other An `any`.
    auto operator=(Any&& other) -> virtual_any& {
        using namespace detail;

        obj = std::move(other);
        vp = box_vptr<use_indirect_vptrs>(acquire_vptr<Registry>(obj));

        return *this;
    }

    //! Assign from a value.
    //!
    //! Stores `value` in the `any`, and sets the v-table pointer to the
    //! @ref registry::static_vptr for its type - no hash table lookup is
    //! involved. The type of `value`, stripped from reference and
    //! cv-qualifiers, is registered automatically in `Registry`, as a
    //! class derived from `Any`.
    //!
    //! @tparam T The type of the value.
    //! @param value The value to store.
    template<
        typename T,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                IsVirtualAny<std::decay_t<T>> == false>,
        typename = std::enable_if_t<
            !std::is_same_v<std::decay_t<T>, Any> &&
            std::is_constructible_v<Any, T&&>>>
    auto operator=(T&& value) -> virtual_any& {
        using namespace detail;

        (void)&use_any_classes<Registry, Any, std::decay_t<T>>;
        obj = std::forward<T>(value);
        Registry::require_initialized();
        vp = box_vptr<use_indirect_vptrs>(
            Registry::template static_vptr<std::decay_t<T>>);
        BOOST_ASSERT(unbox_vptr(vp) != nullptr);

        return *this;
    }

    //! Construct a value in place.
    //!
    //! Stores a `Class` constructed from `args`, and sets the v-table
    //! pointer to the @ref registry::static_vptr for `Class` - no hash
    //! table lookup is involved. `Class` is registered automatically in
    //! `Registry`, as a class derived from `Any`.
    //!
    //! @tparam Class The type of the value to construct.
    //! @tparam T Types of the arguments to pass to the constructor.
    //! @param args Arguments to pass to the constructor of `Class`.
    //!
    //! @par Example
    //! include:virtual_any.cpp#emplace
    template<class Class, typename... T>
    auto emplace(T&&... args) -> void {
        (void)&detail::use_any_classes<Registry, Any, Class>;
        obj = Class(std::forward<T>(args)...);
        Registry::require_initialized();
        vp = detail::box_vptr<use_indirect_vptrs>(
            Registry::template static_vptr<Class>);
        BOOST_ASSERT(detail::unbox_vptr(vp) != nullptr);
    }

    //! Return a reference to the (non-modifiable) `any`.
    auto get() const -> const Any& {
        return obj;
    }

    //! Return the v-table pointer.
    auto vptr() const -> vptr_type {
        return detail::unbox_vptr(vp);
    }
};

//! Specialize virtual_traits for `const virtual_any&`.
//!
//! Dispatch is on the v-table pointer stored in the `virtual_any`.
//!
//! @tparam Any An `any` type.
//! @tparam Registry A @ref registry.
template<class Any, class Registry>
struct virtual_traits<const virtual_any<Any, Registry>&, Registry> {
    //! The type used for dispatch.
    using virtual_type = Any;

    //! Returns a const reference to the `virtual_any` argument.
    //! @param arg A reference to a `virtual_any`.
    //! @return A const reference to `arg`.
    static auto peek(const virtual_any<Any, Registry>& arg)
        -> const virtual_any<Any, Registry>& {
        return arg;
    }

    //! Returns a *reference* to the v-table pointer for the stored value.
    //!
    //! The `virtual_any` acquired the v-table pointer when it was created,
    //! so no lookup is involved. Registers `Any` - the root class of the
    //! contained types - in `Registry`.
    //!
    //! @param arg A reference to a const `virtual_any`.
    //! @return A reference to the v-table pointer for the stored value.
    static auto vptr(const virtual_any<Any, Registry>& arg)
        -> const vptr_type& {
        (void)&detail::use_any_classes<Registry, Any>;

        return arg.vptr_ref();
    }

    //! Cast to a type.
    //!
    //! If `U` is the `virtual_any` itself (by any reference category),
    //! returns `arg` unchanged. Otherwise, extracts the stored value
    //! using `virtual_traits<const Any&, Registry>::cast`, and registers
    //! `U`, stripped of reference and cv-qualifiers, in `Registry` as a
    //! class derived from `Any`. Since the `any` is not modifiable, `U`
    //! cannot be a mutable reference.
    //!
    //! @tparam U The target type (e.g. `const Dog&`, `Dog`).
    //! @param arg A reference to a const `virtual_any` method argument.
    //! @return The value stored in `arg`, cast to `U`.
    template<typename U>
    static auto cast(const virtual_any<Any, Registry>& arg) -> decltype(auto) {
        if constexpr (
            std::is_same_v<
                std::remove_cv_t<std::remove_reference_t<U>>,
                virtual_any<Any, Registry>>) {
            return (arg);
        } else {
            (void)&detail::use_any_classes<Registry, Any, std::decay_t<U>>;

            return virtual_traits<const Any&, Registry>::template cast<U>(
                arg.obj);
        }
    }
};

//! Specialize virtual_traits for `virtual_any&` (mutable reference).
//!
//! Dispatch is on the v-table pointer stored in the `virtual_any`.
//!
//! @tparam Any An `any` type.
//! @tparam Registry A @ref registry.
template<class Any, class Registry>
struct virtual_traits<virtual_any<Any, Registry>&, Registry> {
    //! The type used for dispatch.
    using virtual_type = Any;

    //! Returns a const reference to the `virtual_any` argument.
    //! @param arg A reference to a `virtual_any`.
    //! @return A const reference to `arg`.
    static auto peek(const virtual_any<Any, Registry>& arg)
        -> const virtual_any<Any, Registry>& {
        return arg;
    }

    //! Returns a *reference* to the v-table pointer for the stored value.
    //!
    //! The `virtual_any` acquired the v-table pointer when it was created,
    //! so no lookup is involved. Registers `Any` - the root class of the
    //! contained types - in `Registry`.
    //!
    //! @param arg A reference to a `virtual_any`.
    //! @return A reference to the v-table pointer for the stored value.
    static auto vptr(const virtual_any<Any, Registry>& arg)
        -> const vptr_type& {
        (void)&detail::use_any_classes<Registry, Any>;

        return arg.vptr_ref();
    }

    //! Cast to a type.
    //!
    //! If `U` is the `virtual_any` itself (by mutable reference), returns
    //! `arg` unchanged. Otherwise, extracts the stored value using
    //! `virtual_traits<Any&, Registry>::cast`, and registers `U`,
    //! stripped of reference and cv-qualifiers, in `Registry` as a class
    //! derived from `Any`. Supports mutable references (e.g. `Dog&`);
    //! modifications through the result are visible through the
    //! `virtual_any`.
    //!
    //! @tparam U The target type (e.g. `Dog&`, `const Dog&`, `Dog`).
    //! @param arg A mutable reference to the `virtual_any` method
    //! argument.
    //! @return The value stored in `arg`, cast to `U`.
    template<typename U>
    static auto cast(virtual_any<Any, Registry>& arg) -> decltype(auto) {
        if constexpr (
            std::is_same_v<
                std::remove_cv_t<std::remove_reference_t<U>>,
                virtual_any<Any, Registry>>) {
            return (arg);
        } else {
            (void)&detail::use_any_classes<Registry, Any, std::decay_t<U>>;

            return virtual_traits<Any&, Registry>::template cast<U>(arg.obj);
        }
    }
};

//! Specialize virtual_traits for `virtual_any&&` (xvalue reference).
//!
//! Dispatch is on the v-table pointer stored in the `virtual_any`.
//!
//! @tparam Any An `any` type.
//! @tparam Registry A @ref registry.
template<class Any, class Registry>
struct virtual_traits<virtual_any<Any, Registry>&&, Registry> {
    //! The type used for dispatch.
    using virtual_type = Any;

    //! Returns a const reference to the `virtual_any` argument.
    //! @param arg A reference to a `virtual_any`.
    //! @return A const reference to `arg`.
    static auto peek(const virtual_any<Any, Registry>& arg)
        -> const virtual_any<Any, Registry>& {
        return arg;
    }

    //! Returns a *reference* to the v-table pointer for the stored value.
    //!
    //! The `virtual_any` acquired the v-table pointer when it was created,
    //! so no lookup is involved. Registers `Any` - the root class of the
    //! contained types - in `Registry`.
    //!
    //! @param arg A reference to a `virtual_any`.
    //! @return A reference to the v-table pointer for the stored value.
    static auto vptr(const virtual_any<Any, Registry>& arg)
        -> const vptr_type& {
        (void)&detail::use_any_classes<Registry, Any>;

        return arg.vptr_ref();
    }

    //! Cast to a type.
    //!
    //! If `U` is the `virtual_any` itself (by rvalue reference), returns
    //! `arg` unchanged. Otherwise, extracts the stored value using
    //! `virtual_traits<Any&&, Registry>::cast`, and registers `U`,
    //! stripped of reference and cv-qualifiers, in `Registry` as a class
    //! derived from `Any`.
    //!
    //! @tparam U The target type (e.g. `Dog&&`, `const Dog&`, `Dog`).
    //! @param arg An rvalue reference to the `virtual_any` method
    //! argument.
    //! @return The value stored in `arg`, cast to `U`.
    template<typename U>
    static auto cast(virtual_any<Any, Registry>&& arg) -> decltype(auto) {
        if constexpr (
            std::is_same_v<
                std::remove_cv_t<std::remove_reference_t<U>>,
                virtual_any<Any, Registry>>) {
            return std::move(arg);
        } else {
            (void)&detail::use_any_classes<Registry, Any, std::decay_t<U>>;

            return virtual_traits<Any&&, Registry>::template cast<U>(
                std::move(arg.obj));
        }
    }
};

//! Reject wrapping a `virtual_any` in a @ref virtual_ptr.
//!
//! A `virtual_any` is already a wide type: it carries the v-table pointer
//! for the value it contains. Wrapping it in a `virtual_ptr` would produce
//! a wide pointer whose v-table is the contained value's, but whose static
//! type is the `virtual_any` - which is deliberately not a registered
//! class. This specialization rejects the combination at compile time,
//! which also covers @ref final_virtual_ptr, since that instantiates the
//! `virtual_ptr` it returns.
//!
//! @tparam Class A specialization of @ref virtual_any, possibly const.
//! @tparam Registry A @ref registry.
template<class Class, class Registry>
class virtual_ptr<
    Class, Registry,
    std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
            IsVirtualAny<std::remove_cv_t<Class>>>> {
    static_assert(
        detail::false_t<Class>,
        "do not wrap a virtual_any in a virtual_ptr: it already carries the "
        "v-table pointer for the value it contains; pass it by reference "
        "instead");
};

namespace detail {

template<class Any, class Registry>
struct is_virtual<virtual_any<Any, Registry>&> : std::true_type {};

template<class Any, class Registry>
struct is_virtual<const virtual_any<Any, Registry>&> : std::true_type {};

template<class Any, class Registry>
struct is_virtual<virtual_any<Any, Registry>&&> : std::true_type {};

template<class Any, class Registry>
struct parameter_traits<virtual_any<Any, Registry>&, Registry> :
    virtual_traits<virtual_any<Any, Registry>&, Registry> {};

template<class Any, class Registry>
struct parameter_traits<const virtual_any<Any, Registry>&, Registry> :
    virtual_traits<const virtual_any<Any, Registry>&, Registry> {};

template<class Any, class Registry>
struct parameter_traits<virtual_any<Any, Registry>&&, Registry> :
    virtual_traits<virtual_any<Any, Registry>&&, Registry> {};

template<class Any, class Registry, class MethodRegistry>
struct validate_method_parameter<
    virtual_any<Any, Registry>, MethodRegistry, void> : std::false_type {
    static_assert(
        false_t<Any, Registry>, "virtual_any must be passed by reference");
};

template<class Any, class Registry, class MethodRegistry>
struct validate_method_parameter<
    virtual_any<Any, Registry>&, MethodRegistry, void> :
    std::bool_constant<std::is_same_v<Registry, MethodRegistry>> {
    static_assert(
        std::is_same_v<Registry, MethodRegistry>, "registry mismatch");
};

template<class Any, class Registry, class MethodRegistry>
struct validate_method_parameter<
    const virtual_any<Any, Registry>&, MethodRegistry, void> :
    std::bool_constant<std::is_same_v<Registry, MethodRegistry>> {
    static_assert(
        std::is_same_v<Registry, MethodRegistry>, "registry mismatch");
};

template<class Any, class Registry, class MethodRegistry>
struct validate_method_parameter<
    virtual_any<Any, Registry>&&, MethodRegistry, void> :
    std::bool_constant<std::is_same_v<Registry, MethodRegistry>> {
    static_assert(
        std::is_same_v<Registry, MethodRegistry>, "registry mismatch");
};

// A virtual_any method parameter places no compile-time constraint on the
// corresponding overrider parameter: the adjustment is delegated entirely
// to virtual_traits<virtual_any cvref>::cast, like for virtual_<T>
// parameters. The exact-pair specializations disambiguate with the
// generic <T, T> specialization in core.hpp, which is neither more nor
// less specialized than <virtual_any cvref, T2>.

template<class Any, class Registry, typename T2>
struct validate_overrider_parameter<virtual_any<Any, Registry>&, T2, void> :
    std::true_type {};

template<class Any, class Registry>
struct validate_overrider_parameter<
    virtual_any<Any, Registry>&, virtual_any<Any, Registry>&, void> :
    std::true_type {};

template<class Any, class Registry, typename T2>
struct validate_overrider_parameter<
    const virtual_any<Any, Registry>&, T2, void> : std::true_type {};

template<class Any, class Registry>
struct validate_overrider_parameter<
    const virtual_any<Any, Registry>&, const virtual_any<Any, Registry>&,
    void> : std::true_type {};

template<class Any, class Registry, typename T2>
struct validate_overrider_parameter<virtual_any<Any, Registry>&&, T2, void> :
    std::true_type {};

template<class Any, class Registry>
struct validate_overrider_parameter<
    virtual_any<Any, Registry>&&, virtual_any<Any, Registry>&&, void> :
    std::true_type {};

template<class Any, class Registry, typename Q>
struct select_overrider_virtual_type_aux<
    virtual_any<Any, Registry>&, Q, Registry> {
    using type = virtual_type<Q, Registry>;
};

template<class Any, class Registry, typename Q>
struct select_overrider_virtual_type_aux<
    const virtual_any<Any, Registry>&, Q, Registry> {
    using type = virtual_type<Q, Registry>;
};

template<class Any, class Registry, typename Q>
struct select_overrider_virtual_type_aux<
    virtual_any<Any, Registry>&&, Q, Registry> {
    using type = virtual_type<Q, Registry>;
};

} // namespace detail

namespace aliases {
using boost::openmethod::virtual_any;
} // namespace aliases

} // namespace boost::openmethod

#endif
