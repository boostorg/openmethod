// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for
// compile_fail_register_classes_mixed_group.cpp, and compiled as a test.

#include "preamble.hpp"

#if BOOST_OPENMETHOD_HAS_REFLECTION

// tag::content[]
BOOST_OPENMETHOD_REGISTER(register_classes<{^^Node, ^^Plus}, {^^my_registry}>);
// end::content[]

#endif
