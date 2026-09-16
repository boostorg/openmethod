// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: carry conflicting registries

#include <string>

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry {};

struct Animal {
    virtual ~Animal() = default;
};

struct Widget {
    virtual ~Widget() = default;
};

// One parameter carries zoo_registry, the other carries the default registry -
// `virtual_ptr` always carries one, since it is a type in its own right. A
// method lives in one registry; naming it on the declaration settles this.
BOOST_OPENMETHOD(
    poke, (virtual_<const Animal&, zoo_registry>, virtual_ptr<Widget>),
    std::string);

int main() {
    return 0;
}
