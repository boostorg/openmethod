// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: registry mismatch

#include <string>

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry {};
struct other_registry : default_registry {};

struct Animal {
    virtual ~Animal() = default;
    friend auto boost_openmethod_registry(Animal*) -> zoo_registry;
};

// `virtual_ptr<Animal>` carries `zoo_registry`, because that is what `Animal`
// says. Declaring the method in another registry contradicts the parameter -
// the check that already guarded an explicitly spelled `virtual_ptr<A, R>`.
BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), std::string, other_registry);

int main() {
    // The check is a fold in `method`'s class body, so it needs the class to be
    // instantiated. Declaring the method is not enough: gcc and clang
    // instantiate it anyway through the registrar, but MSVC does not, and the
    // file then compiles. Calling it forces the point, on every compiler - like
    // compile_fail_virtual_ptr_different_registries.cpp does.
    Animal animal;
    poke(animal);

    return 0;
}
