// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_registry_ambiguous_bases.cpp,
// and compiled as a test.

#include "preamble.hpp"

// tag::content[]
struct Expr {
    virtual ~Expr() = default;
    friend auto boost_openmethod_registry(Expr*) -> my_registry;
};

struct Term {
    virtual ~Term() = default;
    friend auto boost_openmethod_registry(Term*) -> default_registry;
};

struct Sum : Expr, Term {
    friend auto boost_openmethod_registry(Sum*) -> my_registry;
};

BOOST_OPENMETHOD(postfix, (virtual_ptr<Sum>, std::ostream&), void);
// end::content[]
