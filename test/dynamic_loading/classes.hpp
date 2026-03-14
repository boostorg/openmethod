// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>
#include <memory>

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

#define DEFINE_MAKE_DOG()                                                      \
    auto make_dog() {                                                          \
        auto p = &typeid(Dog);                                                 \
        return boost::openmethod::make_unique_virtual<Dog>();                  \
    }