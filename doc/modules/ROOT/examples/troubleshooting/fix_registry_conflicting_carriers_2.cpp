// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the alternative fix for
// compile_fail_registry_conflicting_carriers.cpp, and compiled as a test.

#include "preamble.hpp"

// tag::content[]
struct Expr {
    virtual ~Expr() = default;
    using boost_openmethod_registry = my_registry;
};

BOOST_OPENMETHOD(
    print, (virtual_<Node&>, virtual_ptr<Expr>, std::ostream&), void);
// end::content[]

void call(Plus& plus, Expr& expr) {
    print(plus, expr, std::cout);
}
