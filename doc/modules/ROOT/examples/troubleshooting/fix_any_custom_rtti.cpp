// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for
// compile_fail_any_custom_rtti.cpp, and compiled as a test.

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/interop/std_any.hpp>

BOOST_OPENMETHOD(postfix, (virtual_<const std::any&>, std::ostream&), void);
// end::content[]

void call(const std::any& node) {
    postfix(node, std::cout);
}
