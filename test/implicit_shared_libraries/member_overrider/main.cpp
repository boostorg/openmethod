// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE implicit_shared_libraries_member_overrider

#include <boost/test/unit_test.hpp>

#include "lib.hpp"
#include "../../test_classes.hpp"

#include <boost/openmethod/initialize.hpp>

#include <string>

using namespace boost::openmethod;

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog);

BOOST_AUTO_TEST_CASE(member_overrider_across_modules) {
    BOOST_TEST(lib_registry_state_id() == default_registry::id());

    auto report = initialize().report;

    // Keeper's overrider is declared in a header both modules include, so each
    // registered its own copy. They are copies of one overrider, not two
    // overriders, and must be consolidated: keeping both would make the Dog
    // cell ambiguous, since neither copy is more specific than the other.
    BOOST_TEST(report.ambiguous == 0u);

    Dog dog;
    BOOST_TEST(std::string(Zoo::poke(dog)) == "woof");
    BOOST_TEST(std::string(lib_poke(dog)) == "woof");
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
