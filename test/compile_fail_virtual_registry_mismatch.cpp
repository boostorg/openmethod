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

struct Animal {
    virtual ~Animal() = default;
};

// The parameter carries zoo_registry - spelled on it, since Animal declares
// nothing - and the method says another. A parameter that carries a registry
// must agree with its method; only one that adopts, `virtual_<const Animal&>`,
// would go along.
BOOST_OPENMETHOD(
    speak, (virtual_<const Animal&, zoo_registry>), std::string,
    default_registry);

int main() {
    // See compile_fail_adl_registry_declared_mismatch.cpp for why the call.
    Animal animal;
    speak(animal);

    return 0;
}
