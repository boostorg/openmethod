// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_IMPLICIT_SHARED_LIBRARIES_DEFAULT_LIB_HPP
#define BOOST_OPENMETHOD_TEST_IMPLICIT_SHARED_LIBRARIES_DEFAULT_LIB_HPP

#include <boost/config.hpp>
#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>

// The library owns the registry state; the executable imports it. The
// visibility attribute goes on the declaration only - see lib.cpp.
#if defined(OWNS_REGISTRY_STATE)
BOOST_OPENMETHOD_EXPORT_REGISTRY(boost::openmethod::default_registry);
#else
BOOST_OPENMETHOD_IMPORT_REGISTRY(boost::openmethod::default_registry);
#endif

// BOOST_SYMBOL_VISIBLE gives these classes' RTTI default visibility even in a
// hidden-visibility build, so their type_id unifies across the executable and
// the shared library. Same rationale as test/dynamic_loading/classes.hpp.
struct BOOST_SYMBOL_VISIBLE Animal {
    virtual ~Animal() = default;
};

struct BOOST_SYMBOL_VISIBLE Dog : Animal {};

struct BOOST_SYMBOL_VISIBLE Cat : Animal {};

// Methods are consolidated across modules at initialize() time, so they need no
// DLL decoration of their own.
BOOST_OPENMETHOD(speak, (boost::openmethod::virtual_ptr<Animal>), const char*);

// lib.cpp defines LIB_SOURCE before including this header, so the library
// exports the entry points below and the executable imports them. Unlike
// test/dynamic_loading, nothing is looked up by name at run time - the
// executable links the library implicitly - so ordinary C++ linkage is enough,
// and the factory below can return a non-POD type directly.
#if defined(LIB_SOURCE)
#define LIB_API BOOST_SYMBOL_EXPORT
#else
#define LIB_API BOOST_SYMBOL_IMPORT
#endif

// The address identifying the registry's shared state, as seen from inside the
// library (see registry::id()). It must be identical to the address the
// executable sees.
LIB_API auto lib_registry_state_id() -> const void*;

// Diagnostic: the &typeid(method<...>) that augment_methods() groups the
// per-module copies of `speak` by, as seen from inside the library.
LIB_API auto lib_speak_method_type_id() -> const void*;

// Dispatch performed inside the library, on an object created in either module.
LIB_API auto lib_speak(boost::openmethod::virtual_ptr<Animal> animal) -> const
    char*;

LIB_API auto lib_make_dog() -> boost::openmethod::unique_virtual_ptr<Animal>;

// An instance of the library's own Stray, a class in an anonymous namespace.
LIB_API auto lib_make_stray() -> boost::openmethod::unique_virtual_ptr<Animal>;

#endif
