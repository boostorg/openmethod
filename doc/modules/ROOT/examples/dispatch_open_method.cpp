// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// clang-format off

// tag::content[]
#include <boost/openmethod.hpp>
#include <iostream>

struct Node {
    virtual ~Node() {}
    virtual int value() const = 0;
};

using boost::openmethod::virtual_ptr;

BOOST_OPENMETHOD(postfix, (virtual_ptr<const Node> node, std::ostream& os), void);

// tag::call_via_ref[]
void call_via_ref(const Node& node, std::ostream& os) {
    postfix(node, os);
}
// end::call_via_ref[]

// tag::call_via_virtual_ptr[]
void call_via_virtual_ptr(virtual_ptr<const Node> node, std::ostream& os) {
    postfix(node, os);
}
// end::call_via_virtual_ptr[]
// end::content[]

int main() {
}
