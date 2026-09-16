// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: registry mismatch

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry {};

struct Animal {
    virtual ~Animal() = default;
    using boost_openmethod_registry = zoo_registry;
};

// A registry listed explicitly wins, but not over a class that declares
// another one: that is a contradiction, not a choice.
BOOST_OPENMETHOD_CLASSES(Animal, default_registry);

int main() {
    return 0;
}
