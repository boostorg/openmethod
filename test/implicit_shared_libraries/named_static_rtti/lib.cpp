// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// This library owns and exports the registry state; main.cpp imports it, as in
// the custom_registry variant.
#define OWNS_REGISTRY_STATE
#define LIB_SOURCE

#include "lib.hpp"
#include "../../test_classes.hpp"

BOOST_OPENMETHOD_INSTANTIATE_REGISTRY(static_registry);

using namespace boost::openmethod;

BOOST_OPENMETHOD_TEST_CLASSES(Object, Animal, Dog, Cat);

namespace {

// main.cpp has a Stray of its own, and the compiler prints the same name for
// both. They are different classes, and must not be merged.
struct Stray : Animal {};

} // namespace

BOOST_OPENMETHOD_CLASSES(Animal, Stray);

// A method the executable knows nothing about. Its virtual parameter is a base
// of Animal, so it takes slot 0 in the v-tables of the library's classes, and
// speak takes slot 1.
BOOST_OPENMETHOD(name, (virtual_ptr<Object>), const char*);

BOOST_OPENMETHOD_OVERRIDE(name, (virtual_ptr<Object>), const char*) {
    return "Object";
}

BOOST_OPENMETHOD_OVERRIDE(name, (virtual_ptr<Dog>), const char*) {
    return "Dog";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Animal>), const char*) {
    return "?";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Dog>), const char*) {
    return "woof";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Stray>), const char*) {
    return "library stray";
}

auto lib_registry_state_id() -> const void* {
    return static_registry::id();
}

auto lib_dog_type_id() -> type_id {
    return static_registry::rtti::static_type<Dog>();
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
