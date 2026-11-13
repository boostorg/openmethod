// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_no_virtual_parameters.cpp,
// and compiled as a test.

#include "preamble.hpp"

// tag::content[]
BOOST_OPENMETHOD(postfix, (virtual_<Node&>, std::ostream&), void);
// end::content[]

void call(Plus& plus) {
    postfix(plus, std::cout);
}
