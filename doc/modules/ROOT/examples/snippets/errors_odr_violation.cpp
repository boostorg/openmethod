// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Linked with odr_violation/other.cpp, which is compiled with the opposite
// setting of BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS (see CMakeLists.txt).

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "error_harness.hpp"

using namespace boost::openmethod;

BOOST_AUTO_TEST_CASE(conflicting_definitions) {
    capture_cerr cerr;
    report_on_cerr();

    reporting([] {
        // tag::init[]
        // aborts with error message:
        // conflicting definitions of boost::openmethod::default_registry
        initialize();
        // end::init[]
    });

    BOOST_TEST(
        without_struct_keys(cerr.str()) ==
        "conflicting definitions of boost::openmethod::default_registry\n");
}
