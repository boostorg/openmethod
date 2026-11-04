// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: virtual_<> is not allowed in overriders

#include "preamble.hpp"

// tag::content[]
struct postfix_id;
using postfix = method<postfix_id, void(virtual_<Node&>, std::ostream&)>;

void postfix_plus(virtual_<Plus&>, std::ostream&);

BOOST_OPENMETHOD_REGISTER(postfix::override<postfix_plus>);
// end::content[]
