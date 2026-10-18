// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: overrider parameter discards const qualifier

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

BOOST_OPENMETHOD(poke, (virtual_<const Animal&>), void);

// The method gives read access to the animal, the overrider asks for write
// access. Without the check, this fails inside the thunk, on a `dynamic_cast`
// that casts away constness.
BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), void) {
}

int main() {
    return 0;
}
