// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// The declarations that the compile_fail_*.cpp examples in this directory
// share. The troubleshooting page shows them once, above the examples.

#ifndef BOOST_OPENMETHOD_DOC_TROUBLESHOOTING_PREAMBLE_HPP
#define BOOST_OPENMETHOD_DOC_TROUBLESHOOTING_PREAMBLE_HPP

// tag::content[]
#include <iostream>

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct Node {
    virtual ~Node() = default;
};

struct Plus : Node {};

struct my_registry : default_registry::with<policies::runtime_checks> {};
// end::content[]

#endif
