// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for errors_final.cpp. Runtime
// checks are on, so that a misused final_virtual_ptr would be reported.
#ifndef BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
#define BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
#endif

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

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog>), void) {
    // ...
}

BOOST_AUTO_TEST_CASE(final_construct) {
    initialize();

    capture_cerr cerr;
    report_on_cerr();
    auto returned = false;

    reporting([&] {
        // tag::use[]
        Dog snoopy;
        Animal& animal = snoopy;

        poke(final_virtual_ptr(snoopy));
        poke(virtual_ptr(animal));
        // end::use[]

        returned = true;
    });

    BOOST_TEST(returned);
    BOOST_TEST(cerr.str().empty());
}
