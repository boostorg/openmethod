// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod/default_registry.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>

struct test_registry : boost::openmethod::default_registry::with<
                           boost::openmethod::policies::runtime_checks,
                           boost::openmethod::policies::throw_error_handler> {};

#define BOOST_OPENMETHOD_DEFAULT_REGISTRY test_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE class_registration_unknown_class_overrider
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() {
    }
};

struct Dog : Animal {};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), void) {
}

BOOST_OPENMETHOD_CLASSES(Animal);

BOOST_AUTO_TEST_CASE(unknown_class_overrider) {
    BOOST_CHECK_THROW(initialize(), missing_class);
}
