// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod/default_registry.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>

struct test_registry
    : boost::openmethod::default_registry::with<
          boost::openmethod::policies::throw_error_handler> {};

#define BOOST_OPENMETHOD_DEFAULT_REGISTRY test_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE runtime_errors_throw_error
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace test_matrices;

BOOST_OPENMETHOD_CLASSES(matrix, dense_matrix, diagonal_matrix);

BOOST_OPENMETHOD(
    times, (virtual_<const matrix&>, virtual_<const matrix&>), void);

BOOST_AUTO_TEST_CASE(throw_error) {
    initialize();

    try {
        times(matrix(), matrix());
        BOOST_FAIL("should have thrown");
    } catch (const std::runtime_error& error) {
        BOOST_TEST(
            std::string(error.what()).find("not implemented") !=
            std::string::npos);
    } catch (...) {
        BOOST_FAIL("wrong exception");
    }
}
