// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: conflicting registry affinities

#include <string>

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry {};
struct garage_registry : default_registry {};

struct Animal {
    virtual ~Animal() = default;
    friend auto boost_openmethod_registry(Animal*) -> zoo_registry;
};

struct Vehicle {
    virtual ~Vehicle() = default;
    friend auto boost_openmethod_registry(Vehicle*) -> garage_registry;
};

// A method cannot span two registries, and its parameters say two different
// things. Naming one on the declaration is the way to settle it.
BOOST_OPENMETHOD(
    collide, (virtual_<const Animal&>, virtual_<const Vehicle&>), std::string);

int main() {
}
