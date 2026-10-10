// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#define LIB_SOURCE
#define OWNS_REGISTRY_STATE

#include "lib.hpp"
#include "../../test_classes.hpp"

using namespace boost::openmethod;

BOOST_OPENMETHOD_INSTANTIATE_REGISTRY(boost::openmethod::default_registry);

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog);

auto lib_registry_state_id() -> const void* {
    return default_registry::id();
}

auto lib_poke(virtual_ptr<Animal> animal) -> const char* {
    return Zoo::poke(animal);
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
