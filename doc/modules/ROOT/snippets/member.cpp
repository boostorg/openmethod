// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <sstream>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "capture.hpp"

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};
struct Cat : Animal {};
struct Dog : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog);

// tag::declare[]
struct Zoo {
    BOOST_OPENMETHOD_MEM(
        poke, (virtual_ptr<Animal> animal, std::ostream& os), void);
};
// end::declare[]

// tag::override[]
class Keeper {
    BOOST_OPENMETHOD_OVERRIDE_MEM(
        Zoo::poke, (virtual_ptr<Cat> animal, std::ostream& os), void) {
        (void)animal;
        os << "hiss";
    }

    BOOST_OPENMETHOD_OVERRIDE_MEM(
        Zoo::poke, (virtual_ptr<Dog> animal, std::ostream& os), void) {
        (void)animal;
        os << "bark";
    }
};
// end::override[]

BOOST_AUTO_TEST_CASE(member_method_examples) {
    initialize();

    capture_cout cout;

    // tag::call[]
    Cat felix;
    Animal& a = felix;
    Dog snoopy;
    Animal& b = snoopy;

    Zoo::poke(a, std::cout); // hiss
    Zoo::poke(b, std::cout); // bark
    // end::call[]

    BOOST_TEST(cout.str() == "hissbark");
}
