// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// tag::content[]
// extensions.cpp

#define OWNS_REGISTRY_STATE

#include "animals.hpp"

#include <string>

using namespace boost::openmethod;

BOOST_OPENMETHOD_INSTANTIATE_REGISTRY(boost::openmethod::default_registry);

BOOST_OPENMETHOD_CLASSES(Animal, Herbivore, Cow, Carnivore, Wolf);

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Animal>, virtual_ptr<Animal>), std::string) {
    return "greet";
}
// end::content[]
