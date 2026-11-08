// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for errors_not_implemented.cpp.

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "error_harness.hpp"

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};
struct Dog : Animal {};
struct Cat : Animal {};

// tag::classes[]
BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Animal>), void) {
    // ...
}

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog>), void) {
    // ...
}
// end::classes[]

BOOST_AUTO_TEST_CASE(default_overrider) {
    auto report = initialize().report;
    BOOST_TEST(report.not_implemented == 0u);

    capture_cerr cerr;
    report_on_cerr();
    auto returned = false;

    reporting([&] {
        // tag::use[]
        Cat felix;
        poke(felix);
        // end::use[]

        returned = true;
    });

    BOOST_TEST(returned);
    BOOST_TEST(cerr.str().empty());
}
