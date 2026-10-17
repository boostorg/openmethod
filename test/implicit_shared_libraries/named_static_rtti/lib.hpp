// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_IMPLICIT_SHARED_LIBRARIES_NAMED_STATIC_RTTI_LIB_HPP
#define BOOST_OPENMETHOD_TEST_IMPLICIT_SHARED_LIBRARIES_NAMED_STATIC_RTTI_LIB_HPP

// Defines static_registry, sets it as the default registry, and exports or
// imports its state depending on OWNS_REGISTRY_STATE. Must come before any
// other openmethod header.
#include "registry.hpp"

#include <boost/openmethod/interop/std_unique_ptr.hpp>

// named_static_rtti treats every class as non-polymorphic, so the classes need
// not be polymorphic. They are visible, as they would have to be under
// std_rtti, but that does not merge their type_ids: those are static variables
// of the library's own templates, which have hidden visibility.
struct BOOST_SYMBOL_VISIBLE Object {};

struct BOOST_SYMBOL_VISIBLE Animal : Object {};

struct BOOST_SYMBOL_VISIBLE Dog : Animal {};

struct BOOST_SYMBOL_VISIBLE Cat : Animal {};

BOOST_OPENMETHOD(speak, (boost::openmethod::virtual_ptr<Animal>), const char*);

#if defined(LIB_SOURCE)
#define LIB_API BOOST_SYMBOL_EXPORT
#else
#define LIB_API BOOST_SYMBOL_IMPORT
#endif

// The address identifying the registry's shared state, as seen from inside the
// library.
LIB_API auto lib_registry_state_id() -> const void*;

// static_type<Dog>(), as the library computes it.
LIB_API auto lib_dog_type_id() -> boost::openmethod::type_id;

// Dispatch performed inside the library, on an object created in either module.
LIB_API auto lib_speak(boost::openmethod::virtual_ptr<Animal> animal) -> const
    char*;

LIB_API auto lib_make_dog() -> boost::openmethod::unique_virtual_ptr<Animal>;

// An instance of the library's own Stray, a class in an anonymous namespace.
LIB_API auto lib_make_stray() -> boost::openmethod::unique_virtual_ptr<Animal>;

#endif
