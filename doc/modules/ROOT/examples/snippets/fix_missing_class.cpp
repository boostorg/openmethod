// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for the errors_missing_class_*
// snippets. Runtime checks are on, so that an unregistered class in the call
// would be reported.
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
struct Bulldog : Dog {};

// tag::classes[]
BOOST_OPENMETHOD_CLASSES(Animal, Dog, Bulldog);

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog>), void) {
    // ...
}
// end::classes[]

BOOST_AUTO_TEST_CASE(all_classes_registered) {
    initialize();

    capture_cerr cerr;
    report_on_cerr();
    auto returned = false;

    reporting([&] {
        // tag::use[]
        Bulldog hector;
        poke(hector);
        // end::use[]

        returned = true;
    });

    BOOST_TEST(returned);
    BOOST_TEST(cerr.str().empty());
}
