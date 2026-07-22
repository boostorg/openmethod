// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <memory>
#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE dispatch_next_fn
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() {
    }
};

struct Dog : Animal {};
struct Bulldog : Dog {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Bulldog);

struct BOOST_OPENMETHOD_ID(poke);
using poke =
    method<BOOST_OPENMETHOD_ID(poke), auto(virtual_<Animal&>)->std::string>;

auto poke_dog(Dog&) -> std::string {
    return "bark";
}

BOOST_OPENMETHOD_REGISTER(poke::override<poke_dog>);

auto poke_bulldog(Bulldog& dog) -> std::string {
    return poke::next<poke_bulldog>(dog) + " and bite back";
}

BOOST_OPENMETHOD_REGISTER(poke::override<poke_bulldog>);

BOOST_AUTO_TEST_CASE(test_next_fn) {
    initialize();

    std::unique_ptr<Animal> snoopy = std::make_unique<Dog>();
    BOOST_TEST(poke::fn(*snoopy) == "bark");

    std::unique_ptr<Animal> hector = std::make_unique<Bulldog>();
    BOOST_TEST(poke::fn(*hector) == "bark and bite back");
}
