// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: the rtti policy does not deem this class polymorphic

#include "preamble.hpp"

// tag::content[]
struct Literal {};

void print(Literal& literal) {
    default_registry::policy<policies::vptr>::dynamic_vptr(literal);
}
// end::content[]
