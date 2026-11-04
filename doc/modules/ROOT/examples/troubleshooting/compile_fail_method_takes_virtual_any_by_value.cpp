// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: virtual_any must be passed by reference

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/interop/std_any.hpp>

BOOST_OPENMETHOD(postfix, (virtual_std_any, std::ostream&), void);
// end::content[]

void call(Plus& plus) {
    virtual_std_any node = plus;
    postfix(node, std::cout);
}
