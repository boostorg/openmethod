// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Without C++26 reflection there is nothing to check, so produce the expected
// diagnostic instead, as test/compile_fail_reflection_*.cpp do.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: a group holds one kind of reflection

#include "preamble.hpp"

#if !BOOST_OPENMETHOD_HAS_REFLECTION
#error a group holds one kind of reflection
#else

// tag::content[]
BOOST_OPENMETHOD_REGISTER(register_classes<{^^Node, ^^Plus, ^^my_registry}>);
// end::content[]

#endif
