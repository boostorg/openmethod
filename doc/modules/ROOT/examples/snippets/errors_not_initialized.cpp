// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// A call before `initialize` is caught by the `runtime_checks` policy, which
// `default_registry` carries only when this symbol is defined.
#ifndef BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
#define BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
#endif

#include <boost/openmethod.hpp>

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

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog>), void) {
    // ...
}
// end::classes[]

BOOST_AUTO_TEST_CASE(call_before_initialize) {
    capture_cerr cerr;
    report_on_cerr();

    reporting([] {
        // tag::use[]
        // initialize() has not been called
        Dog snoopy;

        // aborts with error message: not initialized
        poke(snoopy);
        // end::use[]
    });

    BOOST_TEST(cerr.str() == "not initialized\n");
}
