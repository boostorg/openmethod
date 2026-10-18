// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: virtual_ptr<> takes a class or a smart pointer

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};

BOOST_OPENMETHOD_CLASSES(Animal);

// `virtual_ptr` is itself a pointer: the parameter is `virtual_ptr<Animal>`.
BOOST_OPENMETHOD(poke, (virtual_ptr<Animal*>), void);

int main() {
    Animal animal;
    poke(&animal);

    return 0;
}
