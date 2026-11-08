// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for
// compile_fail_final_virtual_ptr_any.cpp, and compiled as a test.

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/interop/std_any.hpp>

BOOST_OPENMETHOD(postfix, (virtual_std_any&, std::ostream&), void);

void print(virtual_std_any& node) {
    postfix(node, std::cout);
}
// end::content[]
