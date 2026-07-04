// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE n2216_pick_any_ambiguous
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace test_matrices;

BOOST_OPENMETHOD_CLASSES(matrix, dense_matrix);

BOOST_OPENMETHOD(
    times, (virtual_<const matrix&>, virtual_<const matrix&>), string_pair);

BOOST_OPENMETHOD_OVERRIDE(times, (const matrix&, const matrix&), string_pair) {
    return string_pair(MATRIX_MATRIX, NONE);
}

BOOST_OPENMETHOD_OVERRIDE(
    times, (const dense_matrix&, const matrix&), string_pair) {
    return string_pair(DENSE_MATRIX, MATRIX_MATRIX);
}

BOOST_OPENMETHOD_OVERRIDE(
    times, (const matrix&, const dense_matrix&), string_pair) {
    return string_pair(MATRIX_DENSE, MATRIX_MATRIX);
}

BOOST_AUTO_TEST_CASE(pick_any_ambiguous) {
    auto compiler = boost::openmethod::initialize(n2216());
    BOOST_TEST(compiler.report.ambiguous == 1u);

    // use covariant return types to resolve ambiguity.
    dense_matrix left, right;
    auto result = times(left, right);
    BOOST_TEST(result.first == MATRIX_DENSE);
    BOOST_TEST(result.second == MATRIX_MATRIX);
}
