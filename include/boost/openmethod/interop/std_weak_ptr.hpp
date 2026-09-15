// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_INTEROP_WEAK_PTR_HPP
#define BOOST_OPENMETHOD_INTEROP_WEAK_PTR_HPP

#include <boost/openmethod/interop/std_shared_ptr.hpp>
#include <memory>
#include <utility>

namespace boost::openmethod {

template<class Class, class Registry = registry_affinity<Class>>
class weak_virtual_ptr;

namespace detail {

// A weak pointer may refer to an object that no longer exists, so there is
// nothing to dispatch on. `virtual_traits` is deliberately *not* specialized
// for `std::weak_ptr`, and `weak_virtual_ptr` is not a `virtual_ptr`. The
// specializations below only replace the vague diagnostics that would result
// from using either as a virtual parameter with a useful one, in the four
// forms a virtual parameter can take. A `weak_virtual_ptr` that is not wrapped
// in `virtual_` is an ordinary parameter, and needs no specialization.

template<typename T>
struct reject_weak_parameter : std::false_type {
    static_assert(
        false_t<T>,
        "a weak pointer cannot be a virtual parameter; call lock() first");
};

template<typename T, class Registry>
struct validate_method_parameter<virtual_<std::weak_ptr<T>>, Registry, void> :
    reject_weak_parameter<T> {};

template<typename T, class Registry>
struct validate_method_parameter<virtual_<std::weak_ptr<T>&>, Registry, void> :
    reject_weak_parameter<T> {};

template<typename T, class Registry>
struct validate_method_parameter<
    virtual_<const std::weak_ptr<T>&>, Registry, void> :
    reject_weak_parameter<T> {};

template<typename T, class Registry>
struct validate_method_parameter<virtual_<std::weak_ptr<T>&&>, Registry, void> :
    reject_weak_parameter<T> {};

template<typename T, class Registry, class MethodRegistry>
struct validate_method_parameter<
    virtual_<weak_virtual_ptr<T, Registry>>, MethodRegistry, void> :
    reject_weak_parameter<T> {};

template<typename T, class Registry, class MethodRegistry>
struct validate_method_parameter<
    virtual_<weak_virtual_ptr<T, Registry>&>, MethodRegistry, void> :
    reject_weak_parameter<T> {};

template<typename T, class Registry, class MethodRegistry>
struct validate_method_parameter<
    virtual_<const weak_virtual_ptr<T, Registry>&>, MethodRegistry, void> :
    reject_weak_parameter<T> {};

template<typename T, class Registry, class MethodRegistry>
struct validate_method_parameter<
    virtual_<weak_virtual_ptr<T, Registry>&&>, MethodRegistry, void> :
    reject_weak_parameter<T> {};

} // namespace detail

//! Weak pointer to an object, remembering its v-table pointer
//!
//! A `weak_virtual_ptr` tracks an object with a `std::weak_ptr`, and
//! remembers its v-table pointer. It is a storage facility, not a
//! `virtual_ptr`: it cannot be dereferenced, compared, or used as a virtual
//! parameter, because the object may no longer exist. It can be passed to a
//! method as an ordinary parameter. Call `lock()` to obtain a
//! @ref shared_virtual_ptr, then use it as usual. Since the v-table pointer is
//! copied from the weak pointer, `lock()` costs no more than
//! `std::weak_ptr::lock()`: no hash table lookup is needed.
//!
//! Remembering the v-table pointer is safe with respect to the lifetime of the
//! object: a `std::weak_ptr` keeps the control block alive, so once the object
//! is destroyed, the weak pointer stays expired, and the v-table pointer can
//! never be applied to another object.
//!
//! @note As for any `virtual_ptr`, the remembered v-table pointer is
//! invalidated when @ref boost::openmethod::initialize is called again, unless
//! the registry uses @ref policies::indirect_vptr.
//!
//! @par Example
//! include:smart_pointers.cpp#classes;weak_lock
//!
//! @tparam Class The class of the object, possibly cv-qualified
//! @tparam Registry The registry in which `Class` is registered. Defaults to
//! the registry `Class` has an affinity for, see @ref registry_affinity.
//!
//! @see [Smart Pointers](xref:ROOT:smart_pointers.adoc)
template<class Class, class Registry>
class weak_virtual_ptr {
#ifndef __MRDOCS__
    template<class, class>
    friend class weak_virtual_ptr;
#endif

    static constexpr bool use_indirect_vptrs = Registry::has_indirect_vptr;

    std::conditional_t<use_indirect_vptrs, const vptr_type*, vptr_type> vp;
    std::weak_ptr<Class> obj;

    template<class Other>
    static auto vptr_of(const std::shared_ptr<Other>& other) {
        return detail::box_vptr<use_indirect_vptrs>(
            other ? detail::acquire_vptr<Registry>(*other) : detail::null_vptr);
    }

    template<class Other>
    static auto vptr_of(
        const virtual_ptr<std::shared_ptr<Other>, Registry>& other) {
        return detail::virtual_ptr_access<
            virtual_ptr<std::shared_ptr<Other>, Registry>>::boxed_vptr(other);
    }

    // Lock `other` once: it is needed to find the dynamic type of the object,
    // and the `std::weak_ptr` is then constructed from the `std::shared_ptr`,
    // which does not lock again, as construction from a `std::weak_ptr` to a
    // different class would. An expired `other` is copied as is, which keeps
    // its control block - and with it `expired()`, `use_count()` and owner
    // identity - as far as the standard library allows: libstdc++ shares
    // ownership with a source that is expired but not empty, as
    // [util.smartptr.weak.const] requires; libc++ locks first, so an expired
    // source of a *different* class yields an empty weak pointer there.
    template<class Other>
    void assign(const std::weak_ptr<Other>& other) {
        auto locked = other.lock();
        vp = vptr_of(locked);

        if (locked) {
            obj = locked;
        } else {
            obj = other;
        }
    }

  public:
    //! Class pointed to by the `std::weak_ptr`
    using element_type = Class;

    //! Default constructor
    //!
    //! Construct an empty `std::weak_ptr`. Set the v-table pointer to
    //! `nullptr`.
    weak_virtual_ptr() :
        vp(detail::box_vptr<use_indirect_vptrs>(detail::null_vptr)) {
    }

    //! Construct from `nullptr`
    //!
    //! Construct an empty `std::weak_ptr`. Set the v-table pointer to
    //! `nullptr`.
    //!
    //! @param value A `nullptr`.
    explicit weak_virtual_ptr(std::nullptr_t) :
        vp(detail::box_vptr<use_indirect_vptrs>(detail::null_vptr)) {
    }

    weak_virtual_ptr(const weak_virtual_ptr& other) = default;

    weak_virtual_ptr(weak_virtual_ptr&& other) noexcept :
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
    weak_virtual_ptr(
        const virtual_ptr<std::shared_ptr<Other>, Registry>& other) :
        vp(vptr_of(other)), obj(other.pointer()) {
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
    weak_virtual_ptr(const weak_virtual_ptr<Other, Registry>& other) :
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
    weak_virtual_ptr(weak_virtual_ptr<Other, Registry>&& other) noexcept :
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
        typename = std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                                        IsPolymorphic<Other, Registry>>,
        typename = std::enable_if_t<std::is_constructible_v<
            std::weak_ptr<Class>, const std::shared_ptr<Other>&>>>
    weak_virtual_ptr(const std::shared_ptr<Other>& other) :
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
        typename = std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                                        IsPolymorphic<Other, Registry>>,
        typename = std::enable_if_t<std::is_constructible_v<
            std::weak_ptr<Class>, const std::weak_ptr<Other>&>>>
    weak_virtual_ptr(const std::weak_ptr<Other>& other) {
        assign(other);
    }

    //! Assign from `nullptr`
    //!
    //! Reset the `std::weak_ptr`. Set the v-table pointer to `nullptr`.
    //!
    //! @param value A `nullptr`.
    weak_virtual_ptr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    weak_virtual_ptr& operator=(const weak_virtual_ptr& other) = default;

    weak_virtual_ptr& operator=(weak_virtual_ptr&& other) noexcept {
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
    weak_virtual_ptr& operator=(
        const virtual_ptr<std::shared_ptr<Other>, Registry>& other) {
        vp = vptr_of(other);
        obj = other.pointer();
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
    weak_virtual_ptr& operator=(
        const weak_virtual_ptr<Other, Registry>& other) {
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
    weak_virtual_ptr& operator=(
        weak_virtual_ptr<Other, Registry>&& other) noexcept {
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
        typename = std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                                        IsPolymorphic<Other, Registry>>,
        typename = std::enable_if_t<std::is_assignable_v<
            std::weak_ptr<Class>&, const std::shared_ptr<Other>&>>>
    weak_virtual_ptr& operator=(const std::shared_ptr<Other>& other) {
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
        typename = std::enable_if_t<BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
                                        IsPolymorphic<Other, Registry>>,
        typename = std::enable_if_t<std::is_assignable_v<
            std::weak_ptr<Class>&, const std::weak_ptr<Other>&>>>
    weak_virtual_ptr& operator=(const std::weak_ptr<Other>& other) {
        assign(other);
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
        using shared = virtual_ptr<std::shared_ptr<Class>, Registry>;

        if (auto locked = obj.lock()) {
            return detail::virtual_ptr_access<shared>::make(
                std::move(locked), vp);
        }

        return shared();
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

    //! Compare owners with a `weak_virtual_ptr`
    //!
    //! Provide the owner-based ordering that an associative container keyed on
    //! `weak_virtual_ptr` needs. Note that `std::owner_less<void>` accepts
    //! `std::shared_ptr` and `std::weak_ptr` alone in some implementations, so
    //! the comparator is best written as a function object calling
    //! `owner_before`.
    //!
    //! @param other A `weak_virtual_ptr`.
    //!
    //! @return The result of `std::weak_ptr::owner_before` applied to the
    //! `std::weak_ptr` held by `other`.
    template<class Other>
    auto owner_before(
        const weak_virtual_ptr<Other, Registry>& other) const noexcept -> bool {
        return obj.owner_before(other.obj);
    }

    //! Compare owners with a `shared_virtual_ptr`
    //!
    //! @param other A `shared_virtual_ptr`.
    //!
    //! @return The result of `std::weak_ptr::owner_before` applied to the
    //! `std::shared_ptr` held by `other`.
    template<class Other>
    auto owner_before(
        const virtual_ptr<std::shared_ptr<Other>, Registry>& other)
        const noexcept -> bool {
        return obj.owner_before(other.pointer());
    }

    //! Release the reference to the object
    //!
    //! Reset the `std::weak_ptr`. Set the v-table pointer to `nullptr`.
    void reset() noexcept {
        obj.reset();
        vp = detail::box_vptr<use_indirect_vptrs>(detail::null_vptr);
    }

    //! Swap with another `weak_virtual_ptr`
    //!
    //! @param other A `weak_virtual_ptr` to the same class.
    void swap(weak_virtual_ptr& other) noexcept {
        std::swap(vp, other.vp);
        obj.swap(other.obj);
    }

    //! Get the weak pointer to the object
    //!
    //! @par Example
    //! include:smart_pointers.cpp#classes;weak_pointer
    //!
    //! @return A const reference to the `std::weak_ptr`
    auto pointer() const noexcept -> const std::weak_ptr<Class>& {
        return obj;
    }

    //! Get the v-table pointer
    //!
    //! @return A pointer to the v-table remembered when the `weak_virtual_ptr`
    //! was created or assigned, or `nullptr`.
    auto vptr() const {
        return detail::unbox_vptr(this->vp);
    }
};

//! Reject a `virtual_ptr` to a `std::weak_ptr`
//!
//! A `std::weak_ptr` may refer to an object that no longer exists, so a
//! `virtual_ptr` cannot track an object through one. This specialization
//! rejects the combination at compile time, which also covers
//! @ref final_virtual_ptr, since that instantiates the `virtual_ptr` it
//! returns. Use @ref weak_virtual_ptr instead.
//!
//! The specialization steps aside if `virtual_traits` is specialized for
//! `std::weak_ptr`.
//!
//! @tparam Class The class pointed to by the `std::weak_ptr`.
//! @tparam Registry A @ref registry.
template<class Class, class Registry>
class virtual_ptr<
    std::weak_ptr<Class>, Registry,
    std::enable_if_t<
        BOOST_OPENMETHOD_UNLESS_MRDOCS(detail::)
            IsSmartPtr<std::weak_ptr<Class>, Registry> == false>> {
    static_assert(
        detail::false_t<Class>,
        "a std::weak_ptr cannot be wrapped in a virtual_ptr; use "
        "weak_virtual_ptr");
};

namespace aliases {
using boost::openmethod::weak_virtual_ptr;
} // namespace aliases

} // namespace boost::openmethod

#endif
