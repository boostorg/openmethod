// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// No static_assert names this mistake, so match the head of the compiler's own
// message: gcc and clang say "no matching function for call to 'postfix'", MSVC
// "'postfix': no matching overloaded function found".
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: no matching function for call to .*postfix|postfix.*: no matching overloaded function found

// tag::content[]
#include <iostream>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

using boost::openmethod::virtual_ptr;

struct Node {};

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
