// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICY_STD_RTTI_HPP
#define BOOST_OPENMETHOD_POLICY_STD_RTTI_HPP

#include <boost/openmethod/preamble.hpp>

#ifndef BOOST_NO_RTTI
#include <string_view>
#include <typeinfo>
#include <boost/core/demangle.hpp>
#endif

namespace boost::openmethod::policies {

//! Implements the @ref rtti policy using standard RTTI.
//!
//! `std_rtti` implements the `rtti` policy using the standard C++ RTTI system.
//! It is the default RTTI policy.
//!
//! @par Example
//! include:policies.cpp#std_rtti;std_rtti_dispatch
//!
//! @see [Custom RTTI](xref:ROOT:custom_rtti.adoc)
struct std_rtti : rtti {
    //! A RttiFn metafunction.
    //!
    //! @tparam Registry The registry containing this policy.
    template<class Registry>
    struct fn {
#ifndef BOOST_NO_RTTI
        //! Tests if a class is polymorphic.
        //!
        //! Evaluates to `true` if `Class` is a polymorphic class, as defined by
        //! the C++ standard, i.e. a class that contains at least one virtual
        //! function.
        //!
        //! @tparam Class A class.
        template<class Class>
        static constexpr bool is_polymorphic = std::is_polymorphic_v<Class>;

        //! Returns the static @ref type_id of a type.
        //!
        //! Returns `&typeid(Class)`, cast to `type_id`.
        //!
        //! @tparam Class A class.
        //! @return The static type_id of Class.
        template<class Class>
        static auto static_type() -> type_id {
            return &typeid(Class);
        }

        //! Returns the dynamic @ref type_id of an object.
        //!
        //! Returns `&typeid(obj)`, cast to `type_id`.
        //!
        //! @tparam Class A registered class.
        //! @param obj A reference to an instance of `Class`.
        //! @return The type_id of `obj`'s class.
        template<class Class>
        static auto dynamic_type(const Class& obj) -> type_id {
            return &typeid(obj);
        }

        //! Writes a representation of a @ref type_id to a stream.
        //!
        //! Writes the demangled name of the class identified by `type` to
        //! `stream`.
        //!
        //! @tparam Stream A SimpleOutputStream.
        //! @param type The `type_id` to write.
        //! @param stream The stream to write to.
        template<typename Stream>
        static auto type_name(type_id type, Stream& stream) -> void {
            stream << boost::core::demangle(
                reinterpret_cast<const std::type_info*>(type)->name());
        }

        //! Returns a key that uniquely identifies a class.
        //!
        //! C++ does *not* guarantee that there is a single instance of
        //! `std::type_info` per type: a class used by several modules of a
        //! program typically has one per module. `type_index` maps a `type_id`
        //! to a key that compares equal for all the type ids of one class,
        //! which is what @ref initialize uses to group the registrations coming
        //! from different modules.
        //!
        //! The key is the *name*, not the address and not a `std::type_index`.
        //! `std::type_index` would delegate to `std::type_info::operator==`,
        //! and that is only as good as the platform's RTTI uniqueness:
        //! libstdc++ falls back to comparing names, but libc++ on Darwin
        //! compares uniquely-named RTTI by address. There, two modules'
        //! `type_info` objects for the same type compare *unequal* unless the
        //! symbol happens to be exported and coalesced by dyld - which, for a
        //! template like `method<Id, Signature, Registry>`, requires every
        //! template argument to have default visibility as well. Under
        //! `-fvisibility=hidden` that is not the case, the copies are not
        //! grouped, each module's method keeps its own overrider list, and
        //! calls report @ref no_overrider. Comparing names sidesteps the
        //! platform's uniqueness rules entirely.
        //!
        //! @param type A `type_id`.
        //! @return The mangled name of the class identified by `type`.
        static auto type_index(type_id type) -> std::string_view {
            return reinterpret_cast<const std::type_info*>(type)->name();
        }

        //! Casts an object to a type.
        //!
        //! Casts `obj` to a reference to an instance of `D`, using
        //! `dynamic_cast`.
        //!
        //! @tparam D A reference to a subclass of `B`.
        //! @tparam B A registered class.
        //! @param obj A reference to an instance of `B`.
        template<typename D, typename B>
        static auto dynamic_cast_ref(B&& obj) -> D {
            return dynamic_cast<D>(obj);
        }
#endif
    };
};

} // namespace boost::openmethod::policies

#endif
