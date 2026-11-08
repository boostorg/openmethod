// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_shared_ptr_overrider.cpp,
// and compiled as a test.

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/interop/std_shared_ptr.hpp>

BOOST_OPENMETHOD(
    postfix, (virtual_<const std::shared_ptr<Node>&>, std::ostream&), void);

BOOST_OPENMETHOD_OVERRIDE(
    postfix, (const std::shared_ptr<Plus>&, std::ostream& os), void) {
    os << "+";
}
// end::content[]
