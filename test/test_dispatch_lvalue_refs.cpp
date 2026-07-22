// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE dispatch_lvalue_refs
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;
using namespace animals;

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);

BOOST_OPENMETHOD(name, (virtual_<const Animal&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Cat& cat), std::string) {
    return cat.owner + "'s cat " + cat.name;
}

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.owner + "'s dog " + dog.name;
}

BOOST_AUTO_TEST_CASE(cast_args_lvalue_refs) {
    initialize();

    Dog spot("Spot");
    BOOST_TEST(name(spot) == "Bill's dog Spot");

    Cat felix("Felix");
    BOOST_TEST(name(felix) == "Bill's cat Felix");
}
