// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_capture_errors.hpp"

#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/std_shared_ptr.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE runtime_errors_bad_call_type_ids_smart_ptr
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace test_matrices;

using capture = capture_errors<test_registry>;

BOOST_OPENMETHOD_TEST_CLASSES(matrix, dense_matrix, diagonal_matrix);

BOOST_OPENMETHOD(
    times, (shared_virtual_ptr<const matrix>, shared_virtual_ptr<const matrix>),
    void);

BOOST_AUTO_TEST_CASE(bad_call_type_ids_smart_ptr) {
    initialize();
    capture capture;

    try {
        auto a = make_shared_virtual<const diagonal_matrix>();
        auto b = make_shared_virtual<const diagonal_matrix>();
        times(a, b);
        BOOST_FAIL("should have thrown");
    } catch (const no_overrider& error) {
        BOOST_TEST(error.arity == 2u);
        BOOST_TEST(error.types[0] == &typeid(diagonal_matrix));
        BOOST_TEST(error.types[1] == &typeid(diagonal_matrix));
    } catch (...) {
        BOOST_FAIL("wrong exception");
    }
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
