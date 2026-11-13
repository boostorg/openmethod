// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the alternative fix for
// compile_fail_overrider_returns_string.cpp, and compiled as a test.

#include "preamble.hpp"

// tag::content[]
#include <string>

BOOST_OPENMETHOD(value, (virtual_ptr<Node>), std::string);

BOOST_OPENMETHOD_OVERRIDE(value, (virtual_ptr<Plus>), std::string) {
    return "+";
}
// end::content[]
