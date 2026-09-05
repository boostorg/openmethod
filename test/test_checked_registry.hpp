// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_CHECKED_REGISTRY_HPP
#define BOOST_OPENMETHOD_TEST_CHECKED_REGISTRY_HPP

// This header owns the whole registry recipe for the tests that expect a
// registration error to be thrown: the declaration, the
// BOOST_OPENMETHOD_DEFAULT_REGISTRY definition, the library include, and
// `test_registry` itself. Including it first - before anything that pulls in
// core.hpp - is all a test has to do, and there is no ordering left for a
// caller to get wrong.
//
// `runtime_checks` catches what initialize() cannot; `throw_error_handler`
// turns the diagnosis into an exception the test can catch.
struct test_registry;
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY test_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>

struct test_registry :
    boost::openmethod::default_registry::with<
        boost::openmethod::policies::runtime_checks,
        boost::openmethod::policies::throw_error_handler> {};

#endif
