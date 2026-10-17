// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// This library owns and exports the registry state; main.cpp imports it. That
// is the natural direction for implicit linking: the executable already links
// this library, so on Windows the import-library dependency runs the usual way
// (see the "Implicit Linking" section of shared_libraries.adoc).
#define LIB_SOURCE
#define OWNS_REGISTRY_STATE

#include "lib.hpp"
#include "../../test_classes.hpp"

using namespace boost::openmethod;

// The one definition of the shared state. It carries no visibility attribute:
// lib.hpp already declared it exported, and repeating the attribute here is an
// error on GCC.
BOOST_OPENMETHOD_INSTANTIATE_REGISTRY(boost::openmethod::default_registry);

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog, Cat);

namespace {

// main.cpp has a Stray of its own, with the same name. They are different
// classes, and must not be merged.
struct Stray : Animal {};

} // namespace

BOOST_OPENMETHOD_CLASSES(Animal, Stray);

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Animal>), const char*) {
    return "?";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Dog>), const char*) {
    return "woof";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Stray>), const char*) {
    return "library stray";
}

auto lib_speak_method_type_id() -> const void* {
    return &typeid(
        BOOST_OPENMETHOD_TYPE(speak, (virtual_ptr<Animal>), const char*));
}

auto lib_registry_state_id() -> const void* {
    return default_registry::id();
}

auto lib_speak(virtual_ptr<Animal> animal) -> const char* {
    return speak(animal);
}

auto lib_make_dog() -> unique_virtual_ptr<Animal> {
    return make_unique_virtual<Dog>();
}

auto lib_make_stray() -> unique_virtual_ptr<Animal> {
    return make_unique_virtual<Stray>();
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
