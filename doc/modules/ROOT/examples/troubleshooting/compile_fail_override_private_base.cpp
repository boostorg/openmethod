// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: cannot find 'postfix' method that accepts the same arguments as the overrider

// tag::content[]
#include <iostream>

#include <boost/openmethod.hpp>

using boost::openmethod::virtual_ptr;

class Node {
  public:
    virtual ~Node() = default;
};

class Plus : Node {};

BOOST_OPENMETHOD(postfix, (virtual_ptr<Node>, std::ostream&), void);

BOOST_OPENMETHOD_OVERRIDE(
    postfix, (virtual_ptr<Plus>, std::ostream& os), void) {
    os << "+";
}
// end::content[]
