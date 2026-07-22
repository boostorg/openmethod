// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <memory>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE n2216_covariant_return_type
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace test_matrices;

BOOST_OPENMETHOD_CLASSES(matrix, dense_matrix);

BOOST_OPENMETHOD(
    times, (virtual_<const matrix&>, virtual_<const matrix&>),
    std::unique_ptr<matrix>);

BOOST_OPENMETHOD_OVERRIDE(
    times, (const dense_matrix&, const matrix&),
    std::unique_ptr<dense_matrix>) {
    return std::make_unique<dense_matrix>();
}

BOOST_OPENMETHOD_OVERRIDE(
    times, (const matrix&, const dense_matrix&), std::unique_ptr<matrix>) {
    return std::make_unique<matrix>();
}

static_assert(std::is_same_v<
              detail::virtual_type<std::unique_ptr<matrix>, default_registry>,
              matrix>);

BOOST_AUTO_TEST_CASE(covariant_return_type) {
    auto compiler = boost::openmethod::initialize(n2216());
    BOOST_TEST(compiler.report.ambiguous == 0u);

    // use covariant return types to resolve ambiguity.
    dense_matrix left, right;
    auto result = times(left, right);
    BOOST_TEST(result->type == DENSE_MATRIX);
}
