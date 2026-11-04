// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: do not wrap a virtual_any in a virtual_ptr

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/interop/std_any.hpp>

void print(virtual_std_any& node) {
    virtual_ptr<virtual_std_any> p(node);
}
// end::content[]
