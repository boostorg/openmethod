// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the alternative fix for
// compile_fail_registry_mismatch_classes.cpp, and compiled as a test.

#include "preamble.hpp"

// tag::content[]
struct Expr {
    virtual ~Expr() = default;
    using boost_openmethod_registry = my_registry;
};

struct Sum : Expr {};

BOOST_OPENMETHOD_CLASSES(Expr, Sum);
// end::content[]
