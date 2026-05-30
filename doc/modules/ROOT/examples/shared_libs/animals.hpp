// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef ANIMALS_DEFINED
#define ANIMALS_DEFINED

// clang-format off

// animals.hpp

#include <boost/openmethod/preamble.hpp>

// tag::content[]
// The module that owns the registry state defines LIBRARY_NAME and exports it;
// modules that link against or load it import it. This must be set before
// <boost/openmethod/default_registry.hpp> is included. No decoration is needed
// on non-Windows platforms.
#ifdef _WIN32
#ifdef LIBRARY_NAME
#define BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY
#else
#define BOOST_OPENMETHOD_IMPORT_DEFAULT_REGISTRY
#endif
#endif

#include <string>
#include <boost/openmethod.hpp>

struct Animal { virtual ~Animal() {} };
struct Herbivore : Animal {};
struct Carnivore : Animal {};

struct Cow : Herbivore {};
struct Wolf : Carnivore {};

BOOST_OPENMETHOD_CLASSES(Animal, Herbivore, Cow, Carnivore, Wolf);

BOOST_OPENMETHOD(
    meet, (
        boost::openmethod::virtual_ptr<Animal>,
        boost::openmethod::virtual_ptr<Animal>),
    std::string);
// end::content[]

#endif
