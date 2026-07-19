// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "registry.hpp"
#include "classes.hpp"

#include <string>
#include <utility>

// meet returns {own_word, next_word}: the overrider's own word plus the word
// reported by next() (or "n/a" when there is no next overrider). This exercises
// next across the DSO boundary (see overrider.cpp / main.cpp).
using greeting = std::pair<std::string, std::string>;

// Methods are consolidated across modules at initialize() time, so they need no
// DLL decoration of their own.
BOOST_OPENMETHOD(speak, (boost::openmethod::virtual_ptr<Animal>), const char*);

BOOST_OPENMETHOD(
    meet,
    (boost::openmethod::virtual_ptr<Animal>,
     boost::openmethod::virtual_ptr<Animal>),
    greeting);

inline auto call_speak(boost::openmethod::virtual_ptr<Animal> animal) {
    return speak(animal);
}