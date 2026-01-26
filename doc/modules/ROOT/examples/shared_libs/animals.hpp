// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef ANIMALS_DEFINED
#define ANIMALS_DEFINED

// clang-format off

// tag::content[]
// animals.hpp

#include <boost/openmethod/preamble.hpp>

#ifdef BOOST_CLANG
#pragma clang diagnostic ignored "-Wundefined-var-template"
#endif

#ifdef BOOST_GCC
//#pragma GCC diagnostic ignored "-Wundefined-var-template"
#endif

#ifdef LIBRARY_NAME
#define ANIMALS_API boost::openmethod::dllexport
#else
#define ANIMALS_API boost::openmethod::dllimport
#endif

namespace boost::openmethod {
    ANIMALS_API boost_openmethod_declspec(default_registry_attributes);
}

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
    std::string, ANIMALS_API);
// end::content[]

#endif
