// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// `MaxBits = 0` cannot cover two distinct type ids, whatever they are. The
// policy needs BMI2 (see CMakeLists.txt); without it, there is nothing to run.

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/minimal_cover_hash.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "error_harness.hpp"

#include <string>

using namespace boost::openmethod;

#if BOOST_OPENMETHOD_HAS_PEXT

struct Animal {
    virtual ~Animal() = default;
};
struct Dog : Animal {};

// tag::classes[]
struct covered_registry
    : default_registry::with<policies::minimal_cover_hash<0>> {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, covered_registry);
// end::classes[]

BOOST_AUTO_TEST_CASE(too_many_bits) {
    capture_cerr cerr;
    report_on_cerr<covered_registry>();

    reporting([] {
        // tag::init[]
        // aborts with error message:
        // the smallest bit cover of 2 type ids is 1 bits wide, more than the 0 allowed
        initialize<covered_registry>();
        // end::init[]
    });

    BOOST_TEST(
        cerr.str().find("the smallest bit cover of 2 type ids is ") == 0);
    BOOST_TEST(cerr.str().find(" bits wide, more than the 0 allowed\n") !=
               std::string::npos);
}

#else

BOOST_AUTO_TEST_CASE(too_many_bits) {
    BOOST_TEST_MESSAGE("minimal_cover_hash needs BMI2: skipped");
}

#endif
