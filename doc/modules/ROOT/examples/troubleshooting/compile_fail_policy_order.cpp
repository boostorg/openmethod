// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: the type_hash policy must appear before vptr_vector

#include "preamble.hpp"

// tag::content[]
struct hashed_registry :
    default_registry::without<policies::type_hash>::with<
        policies::fast_perfect_hash> {};

BOOST_OPENMETHOD_CLASSES(Node, Plus, hashed_registry);
// end::content[]
