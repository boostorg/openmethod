// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_registry_declared_late.cpp,
// and compiled as a test.

#include "preamble.hpp"

// tag::content[]
struct Expr {
    using boost_openmethod_registry = my_registry;
    virtual ~Expr() = default;
    virtual_ptr<Expr> next;
};

BOOST_OPENMETHOD(postfix, (virtual_ptr<Expr>, std::ostream&), void);
// end::content[]
