// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: a std::weak_ptr cannot be wrapped in a virtual_ptr.*use weak_virtual_ptr

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/interop/std_weak_ptr.hpp>

void watch(std::weak_ptr<Node> node) {
    virtual_ptr<std::weak_ptr<Node>> p(node);
}
// end::content[]
