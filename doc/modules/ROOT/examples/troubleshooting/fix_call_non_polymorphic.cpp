// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_call_non_polymorphic.cpp,
// and compiled as a test.

// tag::content[]
#include <iostream>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

using boost::openmethod::virtual_ptr;

struct Node {
    virtual ~Node() = default;
};

struct Plus : Node {};

BOOST_OPENMETHOD(postfix, (virtual_ptr<Node>, std::ostream&), void);

BOOST_OPENMETHOD_OVERRIDE(
    postfix, (virtual_ptr<Plus>, std::ostream& os), void) {
    os << "+";
}

int main() {
    boost::openmethod::initialize();

    Plus plus;
    postfix(plus, std::cout);
}
// end::content[]
