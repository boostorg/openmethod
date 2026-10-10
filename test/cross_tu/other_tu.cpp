// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// A second translation unit that adds an overrider to Zoo::poke. This is the
// half of the test that fails if Zoo::poke does not name the same method in
// both TUs: the overrider registers with this TU's Zoo::poke, and main.cpp
// calls its own.

#include "member_method.hpp"

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);

class Trainer {
    BOOST_OPENMETHOD_OVERRIDE_MEM(Zoo::poke, (virtual_ptr<Dog>), std::string) {
        return "bark";
    }
};
