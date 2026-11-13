// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_virtual_ptr_of_raw_pointer.cpp,
// and compiled as a test.

#include "preamble.hpp"

// tag::content[]
BOOST_OPENMETHOD(postfix, (virtual_ptr<Node>, std::ostream&), void);

void print(Node* node) {
    postfix(node, std::cout);
}
// end::content[]
