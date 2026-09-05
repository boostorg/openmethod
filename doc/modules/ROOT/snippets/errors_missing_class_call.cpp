// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// The class missing from a *call* is caught by the `runtime_checks` policy,
// which `default_registry` carries only when this symbol is defined.
#define BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS

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

// The registration below is also the `fix` example on the missing_base page,
// hence the nested tag.
// tag::classes[]
// Bulldog is missing
// tag::fix[]
BOOST_OPENMETHOD_CLASSES(Animal, Dog);
// end::fix[]

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog>), void) {
    // ...
}
// end::classes[]

BOOST_AUTO_TEST_CASE(missing_class_in_call) {
    initialize();

    capture_cerr cerr;
    report_on_cerr();

    reporting([] {
        // tag::use[]
        Bulldog hector;

        // aborts with error message: unknown class Bulldog
        poke(hector);
        // end::use[]
    });

    BOOST_TEST(cerr.str().find("Bulldog") != std::string::npos);
}
