// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for errors_hash_search.cpp.

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/minimal_perfect_hash.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "error_harness.hpp"

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};
struct Dog : Animal {};

// tag::classes[]
struct hashed_registry :
    default_registry::with<policies::minimal_perfect_hash<4, 95, 16>> {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, hashed_registry);
// end::classes[]

BOOST_AUTO_TEST_CASE(search_succeeds) {
    capture_cerr cerr;
    report_on_cerr<hashed_registry>();
    auto returned = false;

    reporting([&] {
        // tag::init[]
        initialize<hashed_registry>();
        // end::init[]

        returned = true;
    });

    BOOST_TEST(returned);
    BOOST_TEST(cerr.str().empty());
}
