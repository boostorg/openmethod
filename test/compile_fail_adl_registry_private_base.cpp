// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: ambiguous or inaccessible

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry {};

struct Animal {
    virtual ~Animal() = default;
};

auto boost_openmethod_registry(Animal*) -> zoo_registry;

// The conversion to `Animal*` is inaccessible, so the base's overload cannot
// answer for `Vault` - and neither can the catch-all, which is a worse match.
// The class has to declare its own.
struct Vault : private Animal {};

int main() {
    (void)sizeof(virtual_ptr<Vault>);
}
