// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// extensions.cpp
#include "animals.hpp"

using namespace boost::openmethod;

// tag::content[]
BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Herbivore> a, virtual_ptr<Carnivore> b), std::string) {
    auto p = BOOST_OPENMETHOD_TYPE(
        meet, (virtual_ptr<Animal>, virtual_ptr<Animal>),
        std::string)::next<fn>;
    BOOST_ASSERT(p);
    BOOST_ASSERT(p(a, b) == "greet");
    return "run";
}

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Carnivore>, virtual_ptr<Herbivore>), std::string) {
    return "hunt";
}

struct Tiger : Carnivore {};

BOOST_OPENMETHOD_CLASSES(Tiger, Carnivore);

extern "C" {
BOOST_SYMBOL_EXPORT auto make_tiger() -> Animal* {
    BOOST_ASSERT(default_registry::static_vptr<Carnivore> != nullptr);
    return new Tiger;
}
}
// end::content[]
