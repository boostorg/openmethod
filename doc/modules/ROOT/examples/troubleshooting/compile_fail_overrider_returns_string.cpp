// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: overrider return type is not convertible to method return type

#include <string>

#include "preamble.hpp"

// tag::content[]
BOOST_OPENMETHOD(value, (virtual_ptr<Node>), int);

BOOST_OPENMETHOD_OVERRIDE(value, (virtual_ptr<Plus>), std::string) {
    return "+";
}
// end::content[]
