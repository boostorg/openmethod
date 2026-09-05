// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE implicit_shared_libraries_custom_registry

#include <boost/test/unit_test.hpp>

// OWNS_REGISTRY_STATE is not defined here, so registry.hpp (via lib.hpp)
// declares the state imported: this module uses the state the library owns.
#include "lib.hpp"
#include "../../test_classes.hpp"

#include <boost/openmethod/initialize.hpp>

#include <string>

using namespace boost::openmethod;

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog, Cat, Cow);

// Registered by the executable; the library knows nothing about it.
BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Cat>), const char*) {
    return "meow";
}

// The library is linked implicitly, so its static constructors have already run
// by the time we get here: a single initialize() sets up dispatch tables that
// include both modules' contributions.
BOOST_AUTO_TEST_CASE(shared_registry_state) {
    // The registry's state must be one symbol, shared by both modules, not a
    // private copy per module. Here that sharing rests entirely on the
    // BOOST_OPENMETHOD_{EXPORT,IMPORT}_REGISTRY pair in registry.hpp.
    BOOST_TEST(lib_registry_state_id() == custom_registry::id());

    initialize();

    auto lib_dog = lib_make_dog();
    auto exe_dog = make_unique_virtual<Dog>();
    BOOST_TEST(lib_dog.vptr() == exe_dog.vptr());

    // The library's Dog overrider is visible to the executable...
    BOOST_TEST(std::string(speak(lib_dog)) == "woof");
    BOOST_TEST(std::string(lib_speak(exe_dog)) == "woof");

    // ...and the executable's Cat overrider is visible to the library. This is
    // the strong check: it passes only if both modules registered into the same
    // dispatch tables.
    auto cat = make_unique_virtual<Cat>();
    BOOST_TEST(std::string(lib_speak(cat)) == "meow");
    BOOST_TEST(std::string(speak(cat)) == "meow");

    // lib2.cpp is a second translation unit of the same owning library. Its
    // overrider must be in the tables too - and the library only exports the
    // state at all because registry.hpp gave that TU an exported declaration.
    auto cow = lib_make_cow();
    BOOST_TEST(std::string(speak(cow)) == "moo");
    BOOST_TEST(std::string(lib_speak(cow)) == "moo");

    // A plain Animal falls back to the library's base overrider.
    auto animal = make_unique_virtual<Animal>();
    BOOST_TEST(std::string(lib_speak(animal)) == "?");
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
