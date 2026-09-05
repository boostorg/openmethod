#ifndef BOOST_OPENMETHOD_INPLACE_VPTR_HPP
#define BOOST_OPENMETHOD_INPLACE_VPTR_HPP

#include <boost/openmethod/core.hpp>

// =============================================================================
// inplace_vptr

namespace boost::openmethod {

namespace detail {

void boost_openmethod_bases(...);

// `inplace_vptr_base` declares the affinity that `registry_affinity` reads
// back, so the two are the same question. There is deliberately no catch-all
// here: the one in core.hpp serves both, and a second one returning `void`
// would shadow it for lookups from this namespace.
template<class Class>
using inplace_vptr_registry = registry_affinity<Class>;

template<class>
struct update_vptr_bases;

template<class To, class Class>
void boost_openmethod_update_vptr(Class* obj);

template<class... Bases>
struct update_vptr_bases<mp11::mp_list<Bases...>> {
    template<class To, class Class>
    static void fn(Class* obj) {
        (boost_openmethod_update_vptr<To>(static_cast<Bases*>(obj)), ...);
    }
};

template<class To, class Class>
void boost_openmethod_update_vptr(Class* obj) {
    using registry = inplace_vptr_registry<Class>;
    using bases = decltype(boost_openmethod_bases(obj));

    if constexpr (mp11::mp_size<bases>::value == 0) {
        if constexpr (registry::has_indirect_vptr) {
            obj->boost_openmethod_vptr = &registry::template static_vptr<To>;
        } else {
            obj->boost_openmethod_vptr = registry::template static_vptr<To>;
        }
    } else {
        update_vptr_bases<bases>::template fn<To, Class>(obj);
    }
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif

template<class... Classes>
inline use_classes<Classes...> inplace_vptr_use_classes;

class inplace_vptr_base_tag {};

} // namespace detail

//! Embed a v-table pointer in a class.
//!
//! `inplace_vptr_base` is a
//! [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)
//! mixin that embeds a v-table pointer at the root of a class hierarchy. It
//! also declares a @ref boost_openmethod_vptr free function that returns the
//! v-table pointer stored in the object.
//!
//! `inplace_vptr_base` registers the class in `Registry`. It is not necessary
//! to register the class with @ref use_classes or
//! @ref BOOST_OPENMETHOD_CLASSES.
//!
//! The v-table pointer is obtained directly from the `Registry`\'s @ref
//! static_vptr variable. No hashing is involved. If all the classes in
//! `Registry` inherit from `inplace_vptr_base`, the registry needs not have a
//! @ref policies::vptr policy, nor any policy it depends on (like @ref
//! policies::type_hash).
//!
//! An object that embeds its v-table pointer does not need to be wrapped
//! in a @ref virtual_ptr - the two fill the same goal, fast access to the
//! v-table pointer - and wrapping one is rejected at compile time.
//!
//! If `Registry` contains the @ref has_indirect_vptr policy, the v-table
//! pointer is stored as a pointer to a pointer, and remains valid after a call
//! to @ref initialize.
//!
//! The default value of `Registry` can be changed by defining
//! @ref BOOST_OPENMETHOD_DEFAULT_REGISTRY.
//!
//! @tparam Class The class in which to embed the v-table pointer.
//! @tparam Registry The @ref registry in which `Class` and its derived classes
//! are registered.
//!
//! @par Example
//! include:inplace_vptr.cpp#classes;dispatch
//!
//! @see [Virtual Pointer Alternatives](xref:ROOT:virtual_ptr_alt.adoc)
template<class Class, class Registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY>
class inplace_vptr_base : protected detail::inplace_vptr_base_tag {
    template<class To, class Other>
    friend void detail::boost_openmethod_update_vptr(Other*);
    friend auto boost_openmethod_registry(Class*) -> Registry;
    friend auto boost_openmethod_bases(Class*) -> mp11::mp_list<>;

    std::conditional_t<Registry::has_indirect_vptr, const vptr_type*, vptr_type>
        boost_openmethod_vptr = nullptr;

    friend auto boost_openmethod_vptr(const Class& obj, Registry*) noexcept
        -> vptr_type {
        if constexpr (Registry::has_indirect_vptr) {
            return *obj.boost_openmethod_vptr;
        } else {
            return obj.boost_openmethod_vptr;
        }
    }

  protected:
    //! Set the vptr to `Class`\'s v-table.
    inplace_vptr_base() noexcept {
        (void)&detail::inplace_vptr_use_classes<Class, Registry>;
        detail::boost_openmethod_update_vptr<Class>(static_cast<Class*>(this));
    }

    //! Set the vptr to `nullptr`.
    ~inplace_vptr_base() noexcept {
        boost_openmethod_vptr = nullptr;
    }
};

#ifdef __MRDOCS__
//! Adjust the v-table pointer embedded in a class.
//!
//! `inplace_vptr_derived` is a
//! [CRTP](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern)
//! mixin that adjusts the v-table pointer in a @ref inplace_vptr_base. It can
//! be used only with classes that have @ref inplace_vptr_base as a direct or
//! indirect base class.
//!
//! `inplace_vptr_derived` registers the class and its bases in `Registry`. It
//! is not necessary to register them with @ref use_classes or
//! @ref BOOST_OPENMETHOD_CLASSES.
//!
//! The v-table pointer is obtained directly from the `Registry`\'s @ref
//! static_vptr variable. No hashing is involved. If all the classes in
//! `Registry` inherit from `inplace_vptr_derived`, the registry needs not have a
//! @ref policies::vptr policy, nor any policy it depends on (like @ref
//! policies::type_hash).
//!
//! @ref inplace_vptr_base carries an example.
//!
//! @see @ref inplace_vptr_base
//!
//! @tparam Class The class in which to embed the v-table pointer.
//! @tparam Base A direct base class of `Class`.
//! @tparam MoreBases More direct base classes of `Class`.
//!
//! @see [Virtual Pointer Alternatives](xref:ROOT:virtual_ptr_alt.adoc)
template<class Class, class Base, class... MoreBases>
class inplace_vptr_derived {
  protected:
    //! Set the vptr to `Class`\'s v-table.
    inplace_vptr_derived() noexcept;

    //! Set the vptr in each base class.
    //!
    //! For each base, set its vptr to the base's v-table.
    ~inplace_vptr_derived() noexcept;
};

#else
template<class Class, class Base, class... MoreBases>
class inplace_vptr_derived;
#endif

//! Specialization for a single base class.
//!
//!
//! @see @ref inplace_vptr_derived for documentation.
template<class Class, class Base>
class inplace_vptr_derived<Class, Base> {
    static_assert(
        !detail::is_registry<Base>,
        "registry can be specified only for root classes");

    static_assert(
        std::is_base_of_v<detail::inplace_vptr_base_tag, Base>,
        "class must inherit from inplace_vptr_base");

    template<class To, class Other>
    friend void detail::boost_openmethod_update_vptr(Other*);
    friend auto boost_openmethod_bases(Class*) -> mp11::mp_list<Base>;

  protected:
    //! Set the vptr to `Class`\'s v-table.
    inplace_vptr_derived() noexcept {
        using namespace detail;
        (void)&detail::inplace_vptr_use_classes<
            Class, Base, detail::inplace_vptr_registry<Class>>;
        boost_openmethod_update_vptr<Class>(static_cast<Class*>(this));
    }

    //! Set the vptr in base class.
    ~inplace_vptr_derived() noexcept {
        detail::boost_openmethod_update_vptr<Base>(
            static_cast<Base*>(static_cast<Class*>(this)));
    }
};

//! Specialization for multiple base classes.
//!
//! @see @ref inplace_vptr_derived for documentation.
template<class Class, class Base1, class Base2, class... MoreBases>
class inplace_vptr_derived<Class, Base1, Base2, MoreBases...> {
    static_assert(
        !detail::is_registry<Base1> && !detail::is_registry<Base2> &&
            (!detail::is_registry<MoreBases> && ...),
        "registry can be specified only for root classes");

    // Without this, a `Base1` that is not an inplace_vptr root would silently
    // acquire the default registry from the catch-all - before the hook was
    // unified, `inplace_vptr_registry<Base1>` was `void` and the mistake was a
    // hard error. The single-base specialization asserts the same thing.
    static_assert(
        std::is_base_of_v<detail::inplace_vptr_base_tag, Base1>,
        "class must inherit from inplace_vptr_base");

    friend auto boost_openmethod_registry(Class*)
        -> detail::inplace_vptr_registry<Base1>;
    friend auto boost_openmethod_bases(Class*)
        -> mp11::mp_list<Base1, Base2, MoreBases...>;
    friend auto boost_openmethod_vptr(
        const Class& obj, detail::inplace_vptr_registry<Base1>* registry)
        -> vptr_type {
        return boost_openmethod_vptr(static_cast<const Base1&>(obj), registry);
    }

  protected:
    //! Set the vptr to `Class`\'s v-table.
    inplace_vptr_derived() noexcept {
        (void)&detail::inplace_vptr_use_classes<
            Class, Base1, Base2, MoreBases...,
            detail::inplace_vptr_registry<Base1>>;
        detail::boost_openmethod_update_vptr<Class>(static_cast<Class*>(this));
    }

    //! Set the vptr in each base class.
    //!
    //! For each base, set its vptr to the base's v-table.
    ~inplace_vptr_derived() noexcept {
        auto obj = static_cast<Class*>(this);
        detail::boost_openmethod_update_vptr<Base1>(static_cast<Base1*>(obj));
        detail::boost_openmethod_update_vptr<Base2>(static_cast<Base2*>(obj));
        (detail::boost_openmethod_update_vptr<MoreBases>(
             static_cast<MoreBases*>(obj)),
         ...);
    }
};

namespace aliases {
using boost::openmethod::inplace_vptr_base;
using boost::openmethod::inplace_vptr_derived;
} // namespace aliases

} // namespace boost::openmethod

#endif // BOOST_OPENMETHOD_inplace_vptr_HPP
