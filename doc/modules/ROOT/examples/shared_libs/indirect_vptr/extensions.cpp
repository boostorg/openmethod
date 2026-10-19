// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// tag::content[]
// extensions.cpp

#include "animals.hpp"

#include <string>

using namespace boost::openmethod;

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Herbivore> a, virtual_ptr<Carnivore> b), std::string) {
    auto base = next(a, b);
    return "do not " + base + ", run";
}

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Carnivore>, virtual_ptr<Herbivore>), std::string) {
    return "hunt";
}
// end::content[]
