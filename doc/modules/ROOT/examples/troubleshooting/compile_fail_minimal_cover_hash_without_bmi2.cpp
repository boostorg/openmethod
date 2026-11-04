// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Fires only where the translation unit has no PEXT instruction: x86-64
// without -mbmi2, or any other architecture. Where it has one, there is nothing
// to check, so produce the expected diagnostic instead, as the reflection tests
// do without reflection.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: minimal_cover_hash needs BMI2

#include "preamble.hpp"

#include <boost/openmethod/policies/minimal_cover_hash.hpp>

#if BOOST_OPENMETHOD_HAS_PEXT
#error minimal_cover_hash needs BMI2
#else

// tag::content[]
#include <boost/openmethod/policies/minimal_cover_hash.hpp>

struct covered_registry :
    default_registry::with<policies::minimal_cover_hash<>> {};

BOOST_OPENMETHOD_CLASSES(Node, Plus, covered_registry);
// end::content[]

#endif
