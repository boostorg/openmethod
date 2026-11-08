// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_core_overrider_virtual.cpp,
// and compiled as a test.

#include "preamble.hpp"

// tag::content[]
struct postfix_id;
using postfix = method<postfix_id, void(virtual_<Node&>, std::ostream&)>;

void postfix_plus(Plus&, std::ostream&);

BOOST_OPENMETHOD_REGISTER(postfix::override<postfix_plus>);
// end::content[]
