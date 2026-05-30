// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "registry.hpp"
#include "classes.hpp"

#include <string>

// Methods are consolidated across modules at initialize() time, so they need no
// DLL decoration of their own.
BOOST_OPENMETHOD(
    speak, (boost::openmethod::virtual_ptr<Animal>), const char*);

BOOST_OPENMETHOD(
    meet,
    (boost::openmethod::virtual_ptr<Animal>,
     boost::openmethod::virtual_ptr<Animal>),
    const char*);

inline auto call_speak(boost::openmethod::virtual_ptr<Animal> animal) {
    return speak(animal);
}