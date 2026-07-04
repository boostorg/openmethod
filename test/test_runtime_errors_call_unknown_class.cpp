// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod/default_registry.hpp>

#include "test_capture_errors.hpp"

struct test_registry
    : boost::openmethod::default_registry::with<capture_output> {};

#define BOOST_OPENMETHOD_DEFAULT_REGISTRY test_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE runtime_errors_call_unknown_class
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace test_matrices;

using capture = capture_errors<test_registry>;

// don't register dense_matrix
BOOST_OPENMETHOD_CLASSES(matrix, diagonal_matrix);

BOOST_OPENMETHOD(transpose, (virtual_<const matrix&>), void);

// without any overrider initialize() would do nothing
BOOST_OPENMETHOD_OVERRIDE(transpose, (const matrix&), void) {
}

BOOST_AUTO_TEST_CASE(call_unknown_class) {
    if constexpr (test_registry::has_runtime_checks) {
        {
            initialize();

            capture capture;
            BOOST_CHECK_THROW(transpose(dense_matrix()), missing_class);
            BOOST_TEST(capture().find("unknown class") != std::string::npos);
        }
    }
}
