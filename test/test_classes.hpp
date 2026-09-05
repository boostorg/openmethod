// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_CLASSES_HPP
#define BOOST_OPENMETHOD_TEST_CLASSES_HPP

#include <boost/openmethod/detail/reflection.hpp>

//! Register classes, unless the library can find them by reflection.
//!
//! Expands to @ref BOOST_OPENMETHOD_CLASSES in C++17, and to nothing when the
//! compiler supports C++26 reflection. Tests that are not *about* class
//! registration use this instead of `BOOST_OPENMETHOD_CLASSES`, so that a
//! C++26 run exercises reflection-based registration over the whole suite: the
//! classes go unregistered, and every test still has to pass.
//!
//! Tests that check what happens when a class is *not* registered keep
//! `BOOST_OPENMETHOD_CLASSES`, and do not call
//! `BOOST_OPENMETHOD_REGISTER_CLASSES`: nothing is registered by reflection
//! unless that macro - or `register_classes` - is used, so the class they
//! withhold stays withheld under either standard.

//! Register the classes of the translation unit by reflection, when the
//! compiler supports it - a no-op otherwise.
//!
//! The counterpart of @ref BOOST_OPENMETHOD_TEST_CLASSES: a test lists its
//! classes with that one, for C++17, and ends with this one, for C++26. Last in
//! the file, because the standard wants the registrar after the declarations it
//! selects.
#define BOOST_OPENMETHOD_TEST_REGISTER_CLASSES(...)                            \
    BOOST_OPENMETHOD_REGISTER_CLASSES(__VA_ARGS__)

#if BOOST_OPENMETHOD_HAS_REFLECTION
#define BOOST_OPENMETHOD_TEST_CLASSES(...)
#else
// The build asked for reflection, but this translation unit does not have it:
// it would silently fall back to explicit registration, and the reflection
// tests - which reduce to a no-op without the feature - would pass without
// testing anything. CMake defines the symbol alongside the compiler options
// that enable reflection.
#ifdef BOOST_OPENMETHOD_EXPECT_REFLECTION
#error BOOST_OPENMETHOD_EXPECT_REFLECTION is defined, but the compiler does not provide C++26 reflection
#endif
#define BOOST_OPENMETHOD_TEST_CLASSES(...) BOOST_OPENMETHOD_CLASSES(__VA_ARGS__)
#endif

#endif
