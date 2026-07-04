// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <memory>
#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/std_shared_ptr.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE dispatch_shared_ptr_by_ref
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace animals;

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);

BOOST_OPENMETHOD(
    name, (virtual_<const std::shared_ptr<const Animal>&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(
    name, (const std::shared_ptr<const Cat>& cat), std::string) {
    return cat->owner + "'s cat " + cat->name;
}

BOOST_OPENMETHOD_OVERRIDE(
    name, (const std::shared_ptr<const Dog>& dog), std::string) {
    return dog->owner + "'s dog " + dog->name;
}

BOOST_AUTO_TEST_CASE(cast_args_shared_ptr_by_ref) {
    initialize();

    auto spot = std::make_shared<Dog>("Spot");
    BOOST_TEST(name(spot) == "Bill's dog Spot");

    auto felix = std::make_shared<Cat>("Felix");
    BOOST_TEST(name(felix) == "Bill's cat Felix");
}
