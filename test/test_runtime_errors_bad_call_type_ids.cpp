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

#define BOOST_TEST_MODULE runtime_errors_bad_call_type_ids
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace test_matrices;

using capture = capture_errors<test_registry>;

BOOST_OPENMETHOD_CLASSES(matrix, dense_matrix, diagonal_matrix);

BOOST_OPENMETHOD(
    times, (virtual_ptr<const matrix>, virtual_ptr<const matrix>), void);

BOOST_OPENMETHOD_OVERRIDE(
    times,
    (virtual_ptr<const matrix>, virtual_ptr<const diagonal_matrix>), void) {
}

BOOST_OPENMETHOD_OVERRIDE(
    times,
    (virtual_ptr<const diagonal_matrix>, virtual_ptr<const matrix>), void) {
}

BOOST_AUTO_TEST_CASE(bad_call_type_ids) {
    initialize();
    capture capture;

    try {
        diagonal_matrix a, b;
        times(a, b);
        BOOST_FAIL("should have thrown");
    } catch (const ambiguous_call& error) {
        BOOST_TEST(error.arity == 2u);
        BOOST_TEST(error.types[0] == &typeid(diagonal_matrix));
        BOOST_TEST(error.types[1] == &typeid(diagonal_matrix));
    } catch (...) {
        BOOST_FAIL("wrong exception");
    }
}
