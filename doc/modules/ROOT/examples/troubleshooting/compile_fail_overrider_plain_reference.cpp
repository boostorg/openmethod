// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: virtual_ptr<> is required in overrider in same position as in method

#include "preamble.hpp"

// tag::content[]
BOOST_OPENMETHOD(postfix, (virtual_ptr<Node>, std::ostream&), void);

BOOST_OPENMETHOD_OVERRIDE(postfix, (Plus&, std::ostream& os), void) {
    os << "+";
}
// end::content[]
