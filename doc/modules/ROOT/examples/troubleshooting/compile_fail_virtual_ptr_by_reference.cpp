// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: virtual_ptr<> must be passed by value or by const reference

#include "preamble.hpp"

// tag::content[]
BOOST_OPENMETHOD(postfix, (virtual_ptr<Node>&, std::ostream&), void);
// end::content[]

void call(Plus& plus) {
    virtual_ptr<Node> node(plus);
    postfix(node, std::cout);
}
