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
// registered separately, so the inheritance is never seen
BOOST_OPENMETHOD_CLASSES(Animal);
BOOST_OPENMETHOD_CLASSES(Dog);

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog>), void) {
    // ...
}
// end::classes[]

BOOST_AUTO_TEST_CASE(missing_base_error) {
    capture_cerr cerr;
    report_on_cerr();

    reporting([] {
        // tag::init[]
        // aborts with error message: missing base Animal -<| Dog
        initialize();
        // end::init[]
    });

    BOOST_TEST(cerr.str().find("missing base") != std::string::npos);
}
