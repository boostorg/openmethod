// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

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

// tag::classes[]
BOOST_OPENMETHOD_CLASSES(Animal, Dog);

BOOST_OPENMETHOD(meet, (virtual_ptr<Animal>, virtual_ptr<Animal>), void);

BOOST_OPENMETHOD_OVERRIDE(meet, (virtual_ptr<Animal>, virtual_ptr<Dog>), void) {
    // ...
}

BOOST_OPENMETHOD_OVERRIDE(meet, (virtual_ptr<Dog>, virtual_ptr<Animal>), void) {
    // ...
}
// end::classes[]

BOOST_AUTO_TEST_CASE(ambiguous) {
    initialize();

    capture_cerr cerr;
    report_on_cerr();

    reporting([] {
        // tag::use[]
        Dog snoopy, hector;

        // aborts with error message: ambiguous
        meet(snoopy, hector);
        // end::use[]
    });

    BOOST_TEST(cerr.str() == "ambiguous\n");
}
