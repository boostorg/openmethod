// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: use_classes: a class is incomplete

#include <boost/openmethod.hpp>

struct Animal {
    virtual ~Animal() = default;
};

struct Dog;

// Without the check, this fails in the type traits that compute the
// inheritance relationships.
BOOST_OPENMETHOD_CLASSES(Animal, Dog);

int main() {
    return 0;
}
