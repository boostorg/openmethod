// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>

#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/boost_type_erasure.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "capture.hpp"

namespace te = boost::type_erasure;
using namespace boost::openmethod;

// tag::concept[]
struct Dog {
    std::string name;
};

// The concept must name the Concept it is part of, so the Concept is
// defined as a struct.
struct Dispatchable :
    boost::mpl::vector<
        te::copy_constructible<>, te::relaxed, openmethod_vptr<Dispatchable>> {
};

using erased = te::any<Dispatchable>;

// Binding a value to the `any` registers its type.

BOOST_OPENMETHOD(name, (virtual_<const erased&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}
// end::concept[]

BOOST_AUTO_TEST_CASE(type_erasure_concept_example) {
    initialize();

    {
        capture_cout cout;

        // tag::dispatch[]
        const erased spot(Dog{"Spot"});

        std::cout << name(spot) << "\n"; // Spot the dog
        // end::dispatch[]

        BOOST_TEST(cout.str() == "Spot the dog\n");
    }
}
