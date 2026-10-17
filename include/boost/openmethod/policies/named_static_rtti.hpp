// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICY_NAMED_STATIC_RTTI_HPP
#define BOOST_OPENMETHOD_POLICY_NAMED_STATIC_RTTI_HPP

#include <boost/openmethod/policies/static_rtti.hpp>
#include <boost/openmethod/detail/type_key.hpp>

#include <boost/current_function.hpp>

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>

namespace boost::openmethod::detail {

// The signature of this function, as the compiler prints it, contains the name
// of `T`. Its prefix and suffix are the same for every `T`; they are measured
// on `void`.
template<typename T>
constexpr auto static_rtti_signature() -> const char* {
    return BOOST_CURRENT_FUNCTION;
}

inline constexpr std::string_view static_rtti_probe =
    static_rtti_signature<void>();
inline constexpr std::size_t static_rtti_prefix =
    static_rtti_probe.find("void");

template<typename T>
constexpr auto static_rtti_printed_name() -> std::string_view {
    // Without __PRETTY_FUNCTION__ or __FUNCSIG__, BOOST_CURRENT_FUNCTION falls
    // back on a name that does not contain `T`, and every class would get the
    // same name.
    static_assert(
        static_rtti_prefix != std::string_view::npos,
        "named_static_rtti requires __PRETTY_FUNCTION__ or __FUNCSIG__");

    constexpr std::string_view signature = static_rtti_signature<T>();

    // The name of `T` takes the place of `void` in the probe.
    return signature.substr(
        static_rtti_prefix,
        signature.size() - static_rtti_probe.size() +
            std::string_view("void").size());
}

// MSVC prints the class-key of every class it names, at the head of the name
// and inside template arguments: `struct Box<struct Dog,enum Color>`. Two
// modules that disagree on the key of a class - `class Dog;` in one and
// `struct Dog {...}` in the other, which is only a warning - would print two
// names for it, and the registry would not merge them. Dropping the keys also
// makes the name of a class the same on every compiler, which is what a
// diagnostic shows.
constexpr std::string_view static_rtti_class_keys[] = {
    "struct ", "class ", "union ", "enum "
};

// A keyword is only one where a name cannot continue, or `structural` would
// lose its head.
constexpr auto static_rtti_name_char(char c) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '_';
}

// The length of the class-key starting at `pos`, or zero.
constexpr auto static_rtti_class_key_at(std::string_view name, std::size_t pos)
    -> std::size_t {
    if (pos > 0 && static_rtti_name_char(name[pos - 1])) {
        return 0;
    }

    for (auto key : static_rtti_class_keys) {
        if (name.substr(pos, key.size()) == key) {
            return key.size();
        }
    }

    return 0;
}

constexpr auto static_rtti_stripped_size(std::string_view name) -> std::size_t {
    std::size_t size = 0;

    for (std::size_t pos = 0; pos < name.size();) {
        if (auto key = static_rtti_class_key_at(name, pos)) {
            pos += key;
        } else {
            ++pos;
            ++size;
        }
    }

    return size;
}

// Whether this compiler prints class-keys at all, asked of a class whose own
// name holds no answer. Only MSVC does, and only there is the stripping below
// worth an array per class.
struct static_rtti_probe_class {};

inline constexpr bool static_rtti_prints_class_keys =
    static_rtti_class_key_at(
        static_rtti_printed_name<static_rtti_probe_class>(), 0) != 0;

// The printed name of `T` with the class-keys removed, in storage of its own,
// since it is no longer a substring of the signature. `type_index` compares
// the contents, not the address, so a copy per module costs nothing but space.
template<std::size_t Size>
constexpr auto static_rtti_strip(std::string_view name)
    -> std::array<char, Size> {
    std::array<char, Size> result{};

    for (std::size_t pos = 0, i = 0; pos < name.size();) {
        if (auto key = static_rtti_class_key_at(name, pos)) {
            pos += key;
        } else {
            result[i++] = name[pos++];
        }
    }

    return result;
}

template<typename T>
struct static_rtti_stripped_name {
    static constexpr auto size =
        static_rtti_stripped_size(static_rtti_printed_name<T>());

    static constexpr std::array<char, size> chars =
        static_rtti_strip<size>(static_rtti_printed_name<T>());
};

template<typename T>
constexpr auto static_rtti_type_name() -> std::string_view {
    // Where the compiler prints no class-key, the name is already the answer,
    // and it costs nothing: a view into the signature, which is a string
    // literal the program carries anyway.
    if constexpr (static_rtti_prints_class_keys) {
        using stripped = static_rtti_stripped_name<T>;

        return {stripped::chars.data(), stripped::size};
    } else {
        return static_rtti_printed_name<T>();
    }
}

constexpr auto static_rtti_is_qualifier(char c) -> bool {
    return (c >= 'a' && c <= 'z') || c == ' ' || c == '&';
}

// A class with internal linkage, or none, may print the same name as a
// different class. Only the compilers know which classes they are; this guesses
// it from the marks they leave in the name. GCC prints a local class as
// `f()::L`, or `S::f() const::L`: a scope closed by a parenthesis, maybe
// followed by qualifiers.
constexpr auto static_rtti_marked_local(std::string_view name) -> bool {
    constexpr std::string_view marks[] = {
        "{anonymous}",           // GCC
        "(anonymous namespace)", // Clang
        "`",           // MSVC: `anonymous-namespace', `anonymous namespace'
        "<lambda",     // GCC, MSVC
        "(lambda at ", // Clang
        "<unnamed",    // GCC, MSVC
        "(unnamed ",   // Clang
    };

    for (auto mark : marks) {
        if (name.find(mark) != std::string_view::npos) {
            return true;
        }
    }

    for (
        auto pos = name.find("::"); pos != std::string_view::npos;
        pos = name.find("::", pos + 2)) {
        auto i = pos;

        while (i > 0 && static_rtti_is_qualifier(name[i - 1])) {
            --i;
        }

        if (i > 0 && name[i - 1] == ')') {
            return true;
        }
    }

    return false;
}

// Whether the copies of `T`'s type_id in different modules designate the same
// type, and can be merged by `name`, the name of `T`. Called at run time: the
// scan of the name, evaluated at compile time, would cost more than the rest of
// the registration of a class.
template<typename T>
auto static_rtti_shareable(std::string_view name) -> bool {
#if BOOST_OPENMETHOD_HAS_REFLECTION
    if constexpr (
        std::is_class_v<T> || std::is_union_v<T> || std::is_enum_v<T>) {
        return std::meta::has_external_linkage(^^T);
    }
#endif

    // Without reflection; or a function or pointer type, which has no linkage
    // of its own, but may involve a class that has internal linkage.
    return !static_rtti_marked_local(name);
}

} // namespace boost::openmethod::detail

namespace boost::openmethod::policies {

//! Implementation of the `rtti` policy that identifies classes by name.
//!
//! `named_static_rtti` is a @ref static_rtti that works across modules. It
//! categorizes all types as non-polymorphic, and its type ids are addresses
//! that differ from one module to the next, like those of @ref static_rtti. In
//! addition, `type_index` maps them to the name of the class, as the compiler
//! prints it, so the registry merges the copies of a class coming from
//! different modules.
//!
//! All the modules must be built with the same compiler, because the names
//! must be spelled the same way. Dispatch does not use names at all. They are
//! read before `main`, once per class in each module, as the class registers
//! itself, and compared once, by @ref initialize.
//!
//! A class that has internal linkage, or none, is not shared: it is identified
//! by its `type_id`, as with @ref static_rtti. This covers the classes defined
//! in an anonymous namespace, local classes, lambdas, unnamed classes, and the
//! templates instantiated with any of these. With C++26 reflection, the policy
//! asks the compiler which classes they are. Otherwise, it recognizes them from
//! the marks that the compiler leaves in their names.
//!
//! @warning Clang and MSVC leave no mark in the name of a local class. Clang
//! prints it unqualified, and MSVC prints it as if the function were a
//! namespace (`main::Local`). Without reflection, two local classes that print
//! the same name are taken for the same class, and so is a local class that
//! prints the same name as a class at namespace scope. So is a class template
//! instantiated with an entity that has internal linkage, such as the address
//! of a `static` variable, with all compilers.
//!
//! @see [Custom RTTI](xref:ROOT:custom_rtti.adoc)
struct named_static_rtti : rtti {
    //! A RttiFn metafunction.
    //!
    //! @tparam Registry The registry containing this policy.
    template<class Registry>
    struct fn : static_rtti::fn<Registry> {
        //! Returns the @ref type_id of `Class`.
        //!
        //! @tparam Class A class.
        template<class Class>
        static auto static_type() -> type_id {
            using namespace detail;

            // The type_id points to the key: the name of the class, and, if
            // the class cannot be shared, the address of the key itself,
            // which sets it apart from another class with the same name.
            constexpr auto name = static_rtti_type_name<Class>();
            static const type_key id{
                name, static_rtti_shareable<Class>(name) ? nullptr : &id
            };

            return &id;
        }

        //! Returns a key that identifies a class across modules.
        //!
        //! @param type A `type_id`.
        //! @return The name of the class, and `type` itself if the class
        //! cannot be shared.
        static auto type_index(type_id type) {
            return *static_cast<const detail::type_key*>(type);
        }

        //! Writes the name of a class to a stream.
        //!
        //! @param type A `type_id`.
        //! @param stream A stream to write to.
        template<typename Stream>
        static void type_name(type_id type, Stream& stream) {
            stream << static_cast<const detail::type_key*>(type)->name;
        }
    };
};

} // namespace boost::openmethod::policies

#endif
