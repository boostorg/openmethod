// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// This library owns and exports the registry state; main.cpp imports it. That
// is the natural direction for implicit linking: the executable already links
// this library, so on Windows the import-library dependency runs the usual way
// (see the "Implicit Linking" section of shared_libraries.adoc).
//
// OWNS_REGISTRY_STATE selects the exported declaration in registry.hpp; it is
// defined by every translation unit of this library (see also lib2.cpp).
#define OWNS_REGISTRY_STATE
#define LIB_SOURCE

#include "lib.hpp"
#include "../../test_classes.hpp"

// The export is an explicit instantiation *definition*, so a program may
// contain only one: it belongs in a .cpp, never in a header, and this is that
// one place. The matching exported *declaration* in registry.hpp has already
// been seen by this point, in this and every other translation unit of the
// library, which is what keeps the symbol at default visibility.
BOOST_OPENMETHOD_INSTANTIATE_REGISTRY(custom_registry);

using namespace boost::openmethod;

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog, Cat, Cow);

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Animal>), const char*) {
    return "?";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Dog>), const char*) {
    return "woof";
}

auto lib_registry_state_id() -> const void* {
    return custom_registry::id();
}

auto lib_speak(virtual_ptr<Animal> animal) -> const char* {
    return speak(animal);
}

auto lib_make_dog() -> unique_virtual_ptr<Animal> {
    return make_unique_virtual<Dog>();
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
