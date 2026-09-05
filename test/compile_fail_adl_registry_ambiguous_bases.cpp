// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: ambiguous or inaccessible

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry {};
struct kennel_registry : default_registry {};

struct Animal {
    virtual ~Animal() = default;
};

struct Pet {
    virtual ~Pet() = default;
};

auto boost_openmethod_registry(Animal*) -> zoo_registry;
auto boost_openmethod_registry(Pet*) -> kennel_registry;

// Inherits two different affinities, and says nothing itself.
struct Dog : Animal, Pet {};

int main() {
    (void)sizeof(virtual_ptr<Dog>);
}
