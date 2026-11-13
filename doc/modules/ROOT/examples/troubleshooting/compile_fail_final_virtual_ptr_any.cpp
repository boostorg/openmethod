// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// "use of deleted function" on gcc, "call to deleted function" on clang,
// "attempting to reference a deleted function" on MSVC.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: deleted function

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/interop/std_any.hpp>

void print(std::any& node) {
    final_virtual_ptr(node);
}
// end::content[]
