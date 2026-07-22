// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <memory>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE dispatch_rvalue_refs
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace animals;

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);

BOOST_OPENMETHOD(teleport, (virtual_<Animal&&>), std::unique_ptr<Animal>);

BOOST_OPENMETHOD_OVERRIDE(teleport, (Cat && cat), std::unique_ptr<Animal>) {
    return std::make_unique<Cat>(std::move(cat));
}

BOOST_OPENMETHOD_OVERRIDE(teleport, (Dog && dog), std::unique_ptr<Animal>) {
    return std::make_unique<Dog>(std::move(dog));
}

BOOST_AUTO_TEST_CASE(cast_args_rvalue_refs) {
    initialize();

    {
        Dog spot("Spot");
        auto animal = teleport(std::move(spot));
        BOOST_TEST(animal->name == "Spot");
        BOOST_TEST(spot.name == "");
    }

    {
        Cat felix("Felix");
        auto animal = teleport(std::move(felix));
        BOOST_TEST(animal->name == "Felix");
        BOOST_TEST(felix.name == "");
    }
}
