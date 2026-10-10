// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: unexpected argument after the return type

#include <string>

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct my_registry : default_registry::with<> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, my_registry);

// The method may name a registry...
BOOST_OPENMETHOD(
    poke, (virtual_ptr<Animal, my_registry>), std::string, my_registry);

class Keeper {
    // ...but an overrider takes the method's, and naming one is an error.
    BOOST_OPENMETHOD_OVERRIDE_MEM(
        poke, (virtual_ptr<Dog, my_registry>), std::string, my_registry) {
        return "bark";
    }
};

int main() {
    return 0;
}
