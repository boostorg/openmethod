// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// The #define that selects `static_registry` has to precede
// <boost/openmethod.hpp>, so this example needs a translation unit of its own.

// tag::registry[]
struct static_registry;
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY static_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/static_rtti.hpp>

struct static_registry :
    boost::openmethod::registry<boost::openmethod::policies::static_rtti> {};
// end::registry[]

#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "capture.hpp"

using namespace boost::openmethod::aliases;

// tag::classes[]
// polymorphism not required: there is no RTTI to consult
struct Animal {};
struct Cat : Animal {};
struct Dog : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog);

BOOST_OPENMETHOD(trick, (virtual_ptr<Animal>), std::string);

BOOST_OPENMETHOD_OVERRIDE(trick, (virtual_ptr<Dog>), std::string) {
    return "spin";
}

BOOST_OPENMETHOD_OVERRIDE(trick, (virtual_ptr<Cat>), std::string) {
    return "sulk";
}
// end::classes[]

BOOST_AUTO_TEST_CASE(static_rtti_examples) {
    boost::openmethod::initialize();
    capture_cout cout;

    // tag::dispatch[]
    // the exact class must be known where the pointer is created
    unique_virtual_ptr<Animal> a = make_unique_virtual<Cat>();
    unique_virtual_ptr<Animal> b = make_unique_virtual<Dog>();

    std::cout << trick(a) << "\n"; // sulk
    std::cout << trick(b) << "\n"; // spin
    // end::dispatch[]

    BOOST_TEST(cout.str() == "sulk\nspin\n");
}
