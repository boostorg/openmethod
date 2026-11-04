// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// `MaxSeeds = 0` makes the search fail without trying, whatever the type ids:
// the only deterministic way to provoke a search error in a small program.

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/minimal_perfect_hash.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "error_harness.hpp"

#include <string>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};
struct Dog : Animal {};

// tag::classes[]
struct hashed_registry
    : default_registry::with<policies::minimal_perfect_hash<4, 95, 0>> {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, hashed_registry);
// end::classes[]

BOOST_AUTO_TEST_CASE(search_error) {
    capture_cerr cerr;
    report_on_cerr<hashed_registry>();

    reporting([] {
        // tag::init[]
        // aborts with error message:
        // could not place 2 type ids after trying 0 multipliers
        initialize<hashed_registry>();
        // end::init[]
    });

    BOOST_TEST(
        cerr.str() ==
        "could not place 2 type ids after trying 0 multipliers\n\n");
}
