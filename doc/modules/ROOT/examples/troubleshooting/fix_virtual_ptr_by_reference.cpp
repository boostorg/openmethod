// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_virtual_ptr_by_reference.cpp,
// and compiled as a test.

#include "preamble.hpp"

// tag::content[]
BOOST_OPENMETHOD(postfix, (virtual_ptr<Node>, std::ostream&), void);
// end::content[]

void call(Plus& plus) {
    virtual_ptr<Node> node(plus);
    postfix(node, std::cout);
}
