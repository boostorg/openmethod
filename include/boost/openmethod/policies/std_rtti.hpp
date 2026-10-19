// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICY_STD_RTTI_HPP
#define BOOST_OPENMETHOD_POLICY_STD_RTTI_HPP

#include <boost/openmethod/preamble.hpp>
#include <boost/openmethod/detail/type_key.hpp>

#include <boost/config.hpp>

#include <string_view>
#include <type_traits>

#ifndef BOOST_NO_RTTI
#include <typeindex>
#include <typeinfo>
#include <boost/core/demangle.hpp>
#endif

// Whether std::type_index identifies classes the way initialize() needs: equal
// for the copies of one class in different modules, and different for two
// classes with internal linkage that have the same name. See
// std_rtti::type_index.
//
// _MSC_VER, not BOOST_MSVC: what matters is MSVC's standard library, which
// clang-cl uses too.
#if defined(_MSC_VER) || (defined(BOOST_GCC) && defined(__GLIBCXX__))
#define BOOST_OPENMETHOD_DETAIL_STD_TYPE_INDEX 1
#else
#define BOOST_OPENMETHOD_DETAIL_STD_TYPE_INDEX 0
#endif

namespace boost::openmethod::detail {

// Whether a type name mangled per the Itanium C++ ABI designates a type that
// a different type, in another translation unit, may share the name of. Only
// the marks that cannot occur in an identifier are recognized, so the answer
// does not depend on parsing the name:
//
//   _GLOBAL__N   the name of an anonymous namespace (a reserved identifier)
//   $_<digit>    Clang's name for an unnamed type or a lambda: `$_0`
//   .            GCC's name for an unnamed type: `._anon_0`
//
// A bare `$` is not one of them: GCC and Clang accept it in an identifier
// unless `-fno-dollars-in-identifiers` is given, so a class named `Foo$Impl`
// would be taken for an unnamed one and never merged across modules.
//
// A class local to a static function, or a template instantiated with the
// address of a static variable, is not recognized - as on MSVC, whose
// decorated names do not tell those apart either.
inline auto itanium_unnamed_mark(std::string_view name) -> bool {
    for (
        auto pos = name.find("$_"); pos != std::string_view::npos;
        pos = name.find("$_", pos + 2)) {
        auto next = pos + 2;

        if (next < name.size() && name[next] >= '0' && name[next] <= '9') {
            return true;
        }
    }

    return false;
}

inline auto itanium_internal_linkage(std::string_view name) -> bool {
    return name.find("_GLOBAL__N") != std::string_view::npos ||
        name.find('.') != std::string_view::npos || itanium_unnamed_mark(name);
}

} // namespace boost::openmethod::detail

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
        //! Returns `&typeid(obj)`, cast to `type_id`. `Class` must be
        //! polymorphic, or `typeid` would yield the static type.
        //!
        //! @tparam Class A registered class.
        //! @param obj A reference to an instance of `Class`.
        //! @return The type_id of `obj`'s class.
        template<class Class>
        static auto dynamic_type(const Class& obj) -> type_id {
            static_assert(is_polymorphic<Class>);

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

        //! Returns a key that identifies a class across modules.
        //!
        //! C++ does *not* guarantee that there is a single instance of
        //! `std::type_info` per type: a class used by several modules of a
        //! program typically has one per module. `type_index` maps a `type_id`
        //! to a key that compares equal for all the type ids of one class,
        //! which is what @ref initialize uses to group the registrations coming
        //! from different modules. The key also tells apart two classes with
        //! the same name in different translation units, when they are defined
        //! in anonymous namespaces, or are unnamed classes or lambdas.
        //!
        //! The type of the key is unspecified; it supports `==`, `<` and
        //! `std::hash`. With MSVC, and with GCC and libstdc++, it is a
        //! `std::type_index`: their `std::type_info` equality compares names,
        //! except where the name of a class is shared by another one, in a
        //! different translation unit. Elsewhere it is not: libc++ compares the
        //! `std::type_info` objects of a class with hidden visibility by
        //! address, so the copies in different modules are unequal, and Clang
        //! does not mark the classes with internal linkage for libstdc++. The
        //! key is then the mangled name, plus the `type_id` if the mangled name
        //! shows an anonymous namespace, an unnamed class, or a lambda.
        //!
        //! @warning Two classes with the same name, local to `static`
        //! functions in different translation units, or two instantiations of
        //! a template on the addresses of `static` variables with the same
        //! name, may be taken for the same class.
        //!
        //! @param type A `type_id`.
        //! @return A key identifying the class of `type`.
        static auto type_index(type_id type) {
            using namespace detail;

            const auto& info = *reinterpret_cast<const std::type_info*>(type);

#if BOOST_OPENMETHOD_DETAIL_STD_TYPE_INDEX
            return std::type_index(info);
#else
            std::string_view name = info.name();

            return type_key{
                name, itanium_internal_linkage(name) ? type : nullptr
            };
#endif
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
