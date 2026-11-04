// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: cast only allowed to a shared_ptr

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/interop/std_shared_ptr.hpp>

BOOST_OPENMETHOD(
    postfix, (virtual_<const std::shared_ptr<Node>&>, std::ostream&), void);

BOOST_OPENMETHOD_OVERRIDE(
    postfix, (std::shared_ptr<Plus>& plus, std::ostream& os), void) {
    os << "+";
}
// end::content[]
