// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_INTEROP_WEAK_PTR_HPP
#define BOOST_OPENMETHOD_INTEROP_WEAK_PTR_HPP

#include <boost/openmethod/interop/std_shared_ptr.hpp>
#include <memory>

namespace boost::openmethod {
namespace detail {

// A weak pointer may refer to an object that no longer exists, so there is
// nothing to dispatch on. `virtual_traits` is deliberately *not* specialized
// for `std::weak_ptr`: that keeps `IsSmartPtr` false, which is what lets the
// hand-written `virtual_ptr` specialization below win over the generic smart
// pointer one. These specializations only replace the vague diagnostics that
// would result from using a weak pointer as a virtual parameter with a useful
// one.

template<typename T, class Registry>
struct validate_method_parameter<virtual_<std::weak_ptr<T>>, Registry, void> :
    std::false_type {
    static_assert(
        false_t<T>,
        "a weak pointer cannot be a virtual parameter; call lock() first");
};

template<typename T, class Registry>
struct validate_method_parameter<
    virtual_ptr<std::weak_ptr<T>, Registry>, Registry, void> : std::false_type {
    static_assert(
        false_t<T>,
        "a weak pointer cannot be a virtual parameter; call lock() first");
};

template<typename T, class Registry>
struct validate_method_parameter<
    virtual_ptr<std::weak_ptr<T>, Registry>&, Registry, void> :
    std::false_type {
    static_assert(
        false_t<T>,
        "a weak pointer cannot be a virtual parameter; call lock() first");
};

template<typename T, class Registry>
struct validate_method_parameter<
    const virtual_ptr<std::weak_ptr<T>, Registry>&, Registry, void> :
    std::false_type {
    static_assert(
        false_t<T>,
        "a weak pointer cannot be a virtual parameter; call lock() first");
};

} // namespace detail

//! Wide pointer combining a `std::weak_ptr` to an object and a pointer to its
//! v-table
//!
//! This specialization of `virtual_ptr` tracks the object with a
//! `std::weak_ptr`, and remembers its v-table pointer. It is a storage
//! facility: unlike other kinds of `virtual_ptr`, it cannot be dereferenced,
//! and it cannot be used as a virtual parameter, because the object may no
//! longer exist. Instead, call `lock()` to obtain a @ref shared_virtual_ptr,
//! then use it as usual. Since the v-table pointer is copied from the weak
//! pointer, `lock()` costs no more than `std::weak_ptr::lock()`: no hash table
//! lookup is needed.
//!
//! Remembering the v-table pointer is safe: a `std::weak_ptr` keeps the
//! control block alive, so once the object is destroyed, the weak pointer
//! stays expired. The v-table pointer can never be applied to another object.
//!
//! @par Example
//! include:smart_pointers.cpp#classes;weak_lock
//!
//! @tparam Class The class of the object, possibly cv-qualified
//! @tparam Registry The registry in which `Class` is registered
//!
//! @see @ref weak_virtual_ptr
//! @see [Smart Pointers](xref:ROOT:smart_pointers.adoc)
template<class Class, class Registry>
class virtual_ptr<std::weak_ptr<Class>, Registry, void> {
#ifndef __MRDOCS__
    template<class, class, typename>
    friend class virtual_ptr;
#endif

    static constexpr bool use_indirect_vptrs = Registry::has_indirect_vptr;

    std::conditional_t<use_indirect_vptrs, const vptr_type*, vptr_type> vp;
    std::weak_ptr<Class> obj;

    template<class Other>
    static auto vptr_of(const std::shared_ptr<Other>& other) {
        return detail::box_vptr<use_indirect_vptrs>(
            other ? detail::acquire_vptr<Registry>(*other) : detail::null_vptr);
    }

  public:
    //! Class pointed to by the `std::weak_ptr`
    using element_type = Class;

    //! Default constructor
    //!
    //! Construct an empty `std::weak_ptr`. Set the v-table pointer to
    //! `nullptr`.
    virtual_ptr() :
        vp(detail::box_vptr<use_indirect_vptrs>(detail::null_vptr)) {
    }

    //! Construct from `nullptr`
    //!
    //! Construct an empty `std::weak_ptr`. Set the v-table pointer to
    //! `nullptr`.
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

    //! Construct from a `shared_virtual_ptr` to a derived class
    //!
    //! Copy the v-table pointer from `other`. Construct the `std::weak_ptr`
    //! from the `std::shared_ptr` held by `other`.
    //!
    //! `Other` is _not_ required to be a polymorphic class: the v-table
    //! pointer is already known.
    //!
    //! @par Example
    //! include:smart_pointers.cpp#classes;weak_lock
    //!
    //! @param other A `shared_virtual_ptr` to an object of a class derived from
    //! `Class`.
    //!
    //! @par Requirements
    //! @li `std::weak_ptr<Class>` must be constructible from
    //! `const std::shared_ptr<Other>&`.
    template<
        class Other,
        typename = std::enable_if_t<std::is_constructible_v<
            std::weak_ptr<Class>, const std::shared_ptr<Other>&>>>
    virtual_ptr(const virtual_ptr<std::shared_ptr<Other>, Registry>& other) :
        vp(other.vp), obj(other.obj) {
    }

    //! Construct from a `weak_virtual_ptr` to a derived class
    //!
    //! Copy the v-table pointer and the `std::weak_ptr` from `other`.
    //!
    //! @param other A `weak_virtual_ptr` to an object of a class derived from
    //! `Class`.
    //!
    //! @par Requirements
    //! @li `std::weak_ptr<Class>` must be constructible from
    //! `const std::weak_ptr<Other>&`.
    template<
        class Other,
        typename = std::enable_if_t<std::is_constructible_v<
            std::weak_ptr<Class>, const std::weak_ptr<Other>&>>>
    virtual_ptr(const virtual_ptr<std::weak_ptr<Other>, Registry>& other) :
        vp(other.vp), obj(other.obj) {
    }

    //! Move-construct from a `weak_virtual_ptr` to a derived class
    //!
    //! Copy the v-table pointer from `other`, and set it to `nullptr` in
    //! `other`. Move the `std::weak_ptr` from `other`.
    //!
    //! @param other A `weak_virtual_ptr` to an object of a class derived from
    //! `Class`.
    //!
    //! @par Requirements
    //! @li `std::weak_ptr<Class>` must be constructible from
    //! `std::weak_ptr<Other>&&`.
    template<
        class Other,
        typename = std::enable_if_t<std::is_constructible_v<
            std::weak_ptr<Class>, std::weak_ptr<Other>&&>>>
    virtual_ptr(virtual_ptr<std::weak_ptr<Other>, Registry>&& other) :
        vp(std::exchange(
            other.vp, detail::box_vptr<use_indirect_vptrs>(detail::null_vptr))),
        obj(std::move(other.obj)) {
    }

    //! Construct from a `std::shared_ptr` to a derived class
    //!
    //! Construct the `std::weak_ptr` from `other`. Set the v-table pointer
    //! according to the dynamic type of `*other`.
    //!
    //! @param other A `std::shared_ptr` to a polymorphic object.
    //!
    //! @par Requirements
    //! @li `Other` must be a polymorphic class, according to the `rtti`
    //! policy of `Registry`.
    //! @li `std::weak_ptr<Class>` must be constructible from
    //! `const std::shared_ptr<Other>&`.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                IsPolymorphic<Other, Registry> &&
            std::is_constructible_v<
                std::weak_ptr<Class>, const std::shared_ptr<Other>&>>>
    virtual_ptr(const std::shared_ptr<Other>& other) :
        vp(vptr_of(other)), obj(other) {
    }

    //! Construct from a `std::weak_ptr` to a derived class
    //!
    //! Construct the `std::weak_ptr` from `other`. Lock `other` to find the
    //! dynamic type of the object, and set the v-table pointer accordingly. If
    //! `other` has expired, the v-table pointer is set to `nullptr`.
    //!
    //! @param other A `std::weak_ptr` to a polymorphic object.
    //!
    //! @par Requirements
    //! @li `Other` must be a polymorphic class, according to the `rtti`
    //! policy of `Registry`.
    //! @li `std::weak_ptr<Class>` must be constructible from
    //! `const std::weak_ptr<Other>&`.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                IsPolymorphic<Other, Registry> &&
            std::is_constructible_v<
                std::weak_ptr<Class>, const std::weak_ptr<Other>&>>>
    virtual_ptr(const std::weak_ptr<Other>& other) :
        vp(vptr_of(other.lock())), obj(other) {
    }

    //! Assign from `nullptr`
    //!
    //! Reset the `std::weak_ptr`. Set the v-table pointer to `nullptr`.
    //!
    //! @param value A `nullptr`.
    virtual_ptr& operator=(std::nullptr_t) {
        obj.reset();
        vp = detail::box_vptr<use_indirect_vptrs>(detail::null_vptr);
        return *this;
    }

    virtual_ptr& operator=(const virtual_ptr& other) = default;

    virtual_ptr& operator=(virtual_ptr&& other) {
        vp = std::exchange(
            other.vp, detail::box_vptr<use_indirect_vptrs>(detail::null_vptr));
        obj = std::move(other.obj);
        return *this;
    }

    //! Assign from a `shared_virtual_ptr` to a derived class
    //!
    //! Copy the v-table pointer from `other`. Assign the `std::weak_ptr` from
    //! the `std::shared_ptr` held by `other`.
    //!
    //! `Other` is _not_ required to be a polymorphic class: the v-table
    //! pointer is already known.
    //!
    //! @param other A `shared_virtual_ptr` to an object of a class derived from
    //! `Class`.
    //!
    //! @par Requirements
    //! @li `std::weak_ptr<Class>` must be assignable from
    //! `const std::shared_ptr<Other>&`.
    template<
        class Other,
        typename = std::enable_if_t<std::is_assignable_v<
            std::weak_ptr<Class>&, const std::shared_ptr<Other>&>>>
    virtual_ptr& operator=(
        const virtual_ptr<std::shared_ptr<Other>, Registry>& other) {
        vp = other.vp;
        obj = other.obj;
        return *this;
    }

    //! Assign from a `weak_virtual_ptr` to a derived class
    //!
    //! Copy the v-table pointer and the `std::weak_ptr` from `other`.
    //!
    //! @param other A `weak_virtual_ptr` to an object of a class derived from
    //! `Class`.
    //!
    //! @par Requirements
    //! @li `std::weak_ptr<Class>` must be assignable from
    //! `const std::weak_ptr<Other>&`.
    template<
        class Other,
        typename = std::enable_if_t<std::is_assignable_v<
            std::weak_ptr<Class>&, const std::weak_ptr<Other>&>>>
    virtual_ptr& operator=(
        const virtual_ptr<std::weak_ptr<Other>, Registry>& other) {
        vp = other.vp;
        obj = other.obj;
        return *this;
    }

    //! Move-assign from a `weak_virtual_ptr` to a derived class
    //!
    //! Copy the v-table pointer from `other`, and set it to `nullptr` in
    //! `other`. Move the `std::weak_ptr` from `other`.
    //!
    //! @param other A `weak_virtual_ptr` to an object of a class derived from
    //! `Class`.
    //!
    //! @par Requirements
    //! @li `std::weak_ptr<Class>` must be assignable from
    //! `std::weak_ptr<Other>&&`.
    template<
        class Other,
        typename = std::enable_if_t<std::is_assignable_v<
            std::weak_ptr<Class>&, std::weak_ptr<Other>&&>>>
    virtual_ptr& operator=(
        virtual_ptr<std::weak_ptr<Other>, Registry>&& other) {
        vp = std::exchange(
            other.vp, detail::box_vptr<use_indirect_vptrs>(detail::null_vptr));
        obj = std::move(other.obj);
        return *this;
    }

    //! Assign from a `std::shared_ptr` to a derived class
    //!
    //! Assign the `std::weak_ptr` from `other`. Set the v-table pointer
    //! according to the dynamic type of `*other`.
    //!
    //! @param other A `std::shared_ptr` to a polymorphic object.
    //!
    //! @par Requirements
    //! @li `Other` must be a polymorphic class, according to the `rtti`
    //! policy of `Registry`.
    //! @li `std::weak_ptr<Class>` must be assignable from
    //! `const std::shared_ptr<Other>&`.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                IsPolymorphic<Other, Registry> &&
            std::is_assignable_v<
                std::weak_ptr<Class>&, const std::shared_ptr<Other>&>>>
    virtual_ptr& operator=(const std::shared_ptr<Other>& other) {
        vp = vptr_of(other);
        obj = other;
        return *this;
    }

    //! Assign from a `std::weak_ptr` to a derived class
    //!
    //! Assign the `std::weak_ptr` from `other`. Lock `other` to find the
    //! dynamic type of the object, and set the v-table pointer accordingly. If
    //! `other` has expired, the v-table pointer is set to `nullptr`.
    //!
    //! @param other A `std::weak_ptr` to a polymorphic object.
    //!
    //! @par Requirements
    //! @li `Other` must be a polymorphic class, according to the `rtti`
    //! policy of `Registry`.
    //! @li `std::weak_ptr<Class>` must be assignable from
    //! `const std::weak_ptr<Other>&`.
    template<
        class Other,
        typename = std::enable_if_t<
            BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                IsPolymorphic<Other, Registry> &&
            std::is_assignable_v<
                std::weak_ptr<Class>&, const std::weak_ptr<Other>&>>>
    virtual_ptr& operator=(const std::weak_ptr<Other>& other) {
        vp = vptr_of(other.lock());
        obj = other;
        return *this;
    }

    //! Lock the weak pointer
    //!
    //! Return a `shared_virtual_ptr` to the object, using the remembered
    //! v-table pointer. No hash table lookup is performed.
    //!
    //! @par Example
    //! include:smart_pointers.cpp#classes;weak_lock
    //!
    //! @return A `shared_virtual_ptr` to the object if it still exists, or an
    //! empty `shared_virtual_ptr` with a `nullptr` v-table pointer otherwise.
    auto lock() const -> virtual_ptr<std::shared_ptr<Class>, Registry> {
        if (auto locked = obj.lock()) {
            return virtual_ptr<std::shared_ptr<Class>, Registry>(
                std::move(locked), vp);
        }

        return virtual_ptr<std::shared_ptr<Class>, Registry>();
    }

    //! Check whether the object still exists
    //!
    //! @return `true` if the `std::weak_ptr` is empty or the object has been
    //! destroyed, `false` otherwise.
    auto expired() const noexcept -> bool {
        return obj.expired();
    }

    //! Get the number of `std::shared_ptr` objects sharing the object
    //!
    //! @return The result of `std::weak_ptr::use_count`.
    auto use_count() const noexcept -> long {
        return obj.use_count();
    }

    //! Release the reference to the object
    //!
    //! Reset the `std::weak_ptr`. Set the v-table pointer to `nullptr`.
    void reset() noexcept {
        obj.reset();
        vp = detail::box_vptr<use_indirect_vptrs>(detail::null_vptr);
    }

    //! Get the weak pointer to the object
    //!
    //! @return A const reference to the `std::weak_ptr`
    auto pointer() const noexcept -> const std::weak_ptr<Class>& {
        return obj;
    }

    //! Get the v-table pointer
    //!
    //! @return A pointer to the v-table remembered when the `virtual_ptr` was
    //! created or assigned, or `nullptr`.
    auto vptr() const {
        return detail::unbox_vptr(this->vp);
    }
};

//! Alias for a `virtual_ptr<std::weak_ptr<T>>`.
//!
//! @par Example
//! include:smart_pointers.cpp#weak_virtual_ptr_alias
//!
//! @see [Smart Pointers](xref:ROOT:smart_pointers.adoc)
template<class Class, class Registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY>
using weak_virtual_ptr = virtual_ptr<std::weak_ptr<Class>, Registry>;

namespace aliases {
using boost::openmethod::weak_virtual_ptr;
} // namespace aliases

} // namespace boost::openmethod

#endif
