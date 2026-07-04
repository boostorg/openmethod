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
#include <boost/openmethod/interop/std_shared_ptr.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE runtime_errors_no_initialization
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace test_matrices;

using capture = capture_errors<test_registry>;

BOOST_OPENMETHOD(transpose, (virtual_ptr<const matrix>), void);

BOOST_OPENMETHOD_OVERRIDE(
    transpose, (virtual_ptr<const diagonal_matrix>), void) {
}

BOOST_AUTO_TEST_CASE(no_initialization) {
    if constexpr (test_registry::has_runtime_checks) {
        // throw during virtual_ptr construction, because of hash table lookup
        {
            capture capture;
            BOOST_CHECK_THROW(
                (shared_virtual_ptr<matrix>{
                    std::make_shared<diagonal_matrix>()}),
                not_initialized);
            BOOST_TEST(capture() == "not initialized\n");
        }

        // throw during method call
        {
            capture capture;
            BOOST_CHECK_THROW(
                transpose(make_shared_virtual<diagonal_matrix>()),
                not_initialized);
        }
    } else {
        try {
            test_registry::require_initialized();
        } catch (not_initialized&) {
            BOOST_TEST_FAIL("should not have thrown in release variant");
        }
    }
}
