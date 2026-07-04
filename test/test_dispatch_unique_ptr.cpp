// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <memory>
#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE dispatch_unique_ptr
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace animals;

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);

BOOST_OPENMETHOD(name, (virtual_<std::unique_ptr<Animal>>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (std::unique_ptr<Cat> cat), std::string) {
    return cat->owner + "'s cat " + cat->name;
}

BOOST_OPENMETHOD_OVERRIDE(name, (std::unique_ptr<Dog> dog), std::string) {
    return dog->owner + "'s dog " + dog->name;
}

BOOST_AUTO_TEST_CASE(cast_args_unique_ptr) {
    initialize();

    auto spot = std::make_unique<Dog>("Spot");
    BOOST_TEST(name(std::move(spot)) == "Bill's dog Spot");
    BOOST_TEST(spot.get() == nullptr);

    auto felix = std::make_unique<Cat>("Felix");
    BOOST_TEST(name(std::move(felix)) == "Bill's cat Felix");
    BOOST_TEST(felix.get() == nullptr);
}
