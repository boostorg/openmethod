// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE implicit_shared_libraries_default_registry

#include <boost/test/unit_test.hpp>

#include "lib.hpp"
#include "../../test_classes.hpp"

#include <boost/openmethod/initialize.hpp>

#include <string>
#include <typeindex>
#include <typeinfo>

using namespace boost::openmethod;

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog, Cat);

// Registered by the executable; the library knows nothing about it.
BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Cat>), const char*) {
    return "meow";
}

// The library is linked implicitly, so its static constructors have already run
// by the time we get here: a single initialize() sets up dispatch tables that
// include both modules' contributions.
BOOST_AUTO_TEST_CASE(shared_registry_state) {
    // The registry's state must be one symbol, shared by both modules, not a
    // private copy per module.
    BOOST_TEST(lib_registry_state_id() == default_registry::id());

    // Regression guard for the macOS bug: the two modules must agree on the
    // identity of `speak`, or augment_methods() will not group their copies,
    // the caller's copy keeps an empty overrider list, and the call below
    // reports no_overrider. type_id itself differs per module - only the rtti
    // policy's type_index is a valid cross-module key.
    {
        auto exe_id = &typeid(
            BOOST_OPENMETHOD_TYPE(speak, (virtual_ptr<Animal>), const char*));
        auto lib_id = lib_speak_method_type_id();
        const auto& exe_ti = *static_cast<const std::type_info*>(exe_id);
        const auto& lib_ti = *static_cast<const std::type_info*>(lib_id);
        BOOST_TEST_MESSAGE(
            "speak method typeid: exe="
            << exe_id << " lib=" << lib_id
            << " same_address=" << (exe_id == lib_id)
            << " type_info_equal=" << (exe_ti == lib_ti) << " type_index_equal="
            << (std::type_index(exe_ti) == std::type_index(lib_ti)));
        // The invariant that matters is the *policy* key: that is what
        // augment_methods() groups by. std::type_index deliberately is NOT
        // asserted - it is false on Darwin, which is the whole reason
        // std_rtti::type_index compares names instead.
        bool method_identity_agrees =
            default_registry::rtti::type_index(exe_id) ==
            default_registry::rtti::type_index(lib_id);
        BOOST_TEST(method_identity_agrees);
    }

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

    // A plain Animal falls back to the library's base overrider.
    auto animal = make_unique_virtual<Animal>();
    BOOST_TEST(std::string(lib_speak(animal)) == "?");
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
