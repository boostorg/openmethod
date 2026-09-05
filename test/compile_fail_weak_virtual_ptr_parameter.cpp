// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: a weak pointer cannot be a virtual parameter; call lock\(\) first

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/std_weak_ptr.hpp>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() {
    }
};
struct Cat : Animal {};

BOOST_OPENMETHOD(poke, (weak_virtual_ptr<Animal>), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (weak_virtual_ptr<Cat>), void) {
}

int main() {
    auto felix = std::make_shared<Cat>();
    poke(weak_virtual_ptr<Cat>(felix));
    return 0;
}
