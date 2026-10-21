// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// clang-format off

// tag::content[]
#include <iostream>

struct Node {
    virtual ~Node() {}
    virtual int value() const = 0;
    virtual void postfix(std::ostream& os) const = 0;
};

// tag::call_virtual_function[]
void call_virtual_function(const Node& node, std::ostream& os) {
    node.postfix(os);
}
// end::call_virtual_function[]
// end::content[]

int main() {
}
