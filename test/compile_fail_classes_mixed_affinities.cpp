// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: carry conflicting registry affinities

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry {};

struct Animal {
    virtual ~Animal() = default;
    using boost_openmethod_registry = zoo_registry;
};

struct Widget {
    virtual ~Widget() = default;
};

// Animal belongs to zoo_registry, Widget declares nothing and so belongs to
// the default one. A class list registers into a single registry and has no
// way to tell which is meant - unlike a method, where a `virtual_` parameter
// over an undeclared class adopts the registry the others agree on. List it.
BOOST_OPENMETHOD_CLASSES(Animal, Widget);

int main() {
    return 0;
}
