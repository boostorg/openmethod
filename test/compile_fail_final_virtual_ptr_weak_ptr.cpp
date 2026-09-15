// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: cannot be wrapped in a virtual_ptr

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/std_weak_ptr.hpp>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() {
    }
};

BOOST_OPENMETHOD_CLASSES(Animal);

int main() {
    auto felix = std::make_shared<Animal>();
    std::weak_ptr<Animal> weak = felix;
    auto p = final_virtual_ptr(weak);
    return 0;
}
