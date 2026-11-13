// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: cannot tell which registry this class belongs to

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

struct Sum : Expr, Term {};

BOOST_OPENMETHOD(postfix, (virtual_ptr<Sum>, std::ostream&), void);
// end::content[]
