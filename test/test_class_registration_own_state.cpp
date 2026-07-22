// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE class_registration_own_state
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

BOOST_AUTO_TEST_CASE(registry_own_state) {
    using r1 = test_registry_<__COUNTER__>;
    using r2 = test_registry_<__COUNTER__>;
    static_assert(!std::is_same_v<r1::registry_type, r2::registry_type>);
    BOOST_CHECK_NE(r1::id(), r2::id());
}
