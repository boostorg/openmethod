// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: different virtual_ptr<> pointer kinds

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/std_shared_ptr.hpp>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), void);

// A `virtual_ptr<Dog>` converts to a `virtual_ptr<Animal>`, so the overrider
// finds the method, but the method has no `shared_ptr` to give it. Without the
// check, this fails on a conversion inside the thunk.
BOOST_OPENMETHOD_OVERRIDE(poke, (shared_virtual_ptr<Dog>), void) {
}

int main() {
    return 0;
}
