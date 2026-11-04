// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: std::shared_ptr cannot be passed by non-const lvalue reference

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/interop/std_shared_ptr.hpp>

BOOST_OPENMETHOD(postfix, (shared_virtual_ptr<Node>&, std::ostream&), void);
// end::content[]

void call(Plus& plus) {
    postfix(plus, std::cout);
}
