// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: overrider return type is not convertible to method return type

#include <string>

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

BOOST_OPENMETHOD(weight, (virtual_ptr<Animal>), int);

// Without the check, this fails on the return statement of the thunk that
// calls the overrider.
BOOST_OPENMETHOD_OVERRIDE(weight, (virtual_ptr<Dog>), std::string) {
    return "heavy";
}

int main() {
    return 0;
}
