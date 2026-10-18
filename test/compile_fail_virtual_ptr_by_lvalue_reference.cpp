// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: virtual_ptr<> must be passed by value or by const reference

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

// A non-const reference cannot bind to the `virtual_ptr` of a derived class,
// and the dispatch would take `virtual_ptr` itself for the class of the object.
BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>&), void);

int main() {
    initialize();
    Dog dog;
    virtual_ptr<Animal> p(dog);
    poke(p);

    return 0;
}
