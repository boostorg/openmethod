// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE dispatch_multi
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace test_matrices;

BOOST_OPENMETHOD_CLASSES(matrix, dense_matrix, diagonal_matrix);

BOOST_OPENMETHOD(
    times, (virtual_<const matrix&>, virtual_<const matrix&>), string_pair);

BOOST_OPENMETHOD(times, (double, virtual_<const matrix&>), string_pair);

BOOST_OPENMETHOD(times, (virtual_<const matrix&>, double), string_pair);

BOOST_OPENMETHOD_OVERRIDE(times, (const matrix&, const matrix&), string_pair) {
    return string_pair(MATRIX_MATRIX, NONE);
}

BOOST_OPENMETHOD_OVERRIDE(
    times, (const diagonal_matrix&, const diagonal_matrix&), string_pair) {
    return string_pair(DIAGONAL_DIAGONAL, MATRIX_MATRIX);
}

BOOST_OPENMETHOD_OVERRIDE(times, (double, const matrix&), string_pair) {
    return string_pair(SCALAR_MATRIX, NONE);
}

BOOST_OPENMETHOD_OVERRIDE(
    times, (double, const diagonal_matrix&), string_pair) {
    return string_pair(SCALAR_DIAGONAL, SCALAR_MATRIX);
}

BOOST_OPENMETHOD_OVERRIDE(
    times, (const diagonal_matrix&, double), string_pair) {
    return string_pair(DIAGONAL_SCALAR, MATRIX_SCALAR);
}

BOOST_OPENMETHOD_OVERRIDE(times, (const matrix&, double), string_pair) {
    return string_pair(MATRIX_SCALAR, NONE);
}

BOOST_AUTO_TEST_CASE(simple) {
    auto report = initialize().report;
    BOOST_TEST(report.not_implemented == 0u);
    BOOST_TEST(report.ambiguous == 0u);

    {
        // pass by const ref
        const matrix& dense = dense_matrix();
        const matrix& diag = diagonal_matrix();
        BOOST_TEST(times(dense, dense) == string_pair(MATRIX_MATRIX, NONE));
        BOOST_TEST(
            times(diag, diag) == string_pair(DIAGONAL_DIAGONAL, MATRIX_MATRIX));
        BOOST_TEST(times(diag, dense) == string_pair(MATRIX_MATRIX, NONE));
        BOOST_TEST(times(2, dense) == string_pair(SCALAR_MATRIX, NONE));
        BOOST_TEST(times(dense, 2) == string_pair(MATRIX_SCALAR, NONE));
        BOOST_TEST(
            times(diag, 2) == string_pair(DIAGONAL_SCALAR, MATRIX_SCALAR));
    }
}
