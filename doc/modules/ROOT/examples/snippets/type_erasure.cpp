// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>

#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/is_empty.hpp>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/boost_type_erasure.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "capture.hpp"

namespace te = boost::type_erasure;
using namespace boost::openmethod;

// tag::classes[]
// `relaxed` implies `typeid_<>`, which dispatch relies on.
using Concept = boost::mpl::vector<te::copy_constructible<>, te::relaxed>;
using erased = te::any<Concept>;

struct Dog {
    std::string name;
};

// The owning `any`, `any<Concept>`, is the common base of the types the
// `any` may bind. The types are registered automatically: naming one in
// an overrider - or storing a value in a `virtual_any` - registers it.
// end::classes[]

// tag::method[]
BOOST_OPENMETHOD(name, (virtual_<const erased&>), std::string);

// An overrider takes the bound value...
BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(name, (const std::string& name), std::string) {
    return name;
}

// ...or the `any` itself, which makes it a catch-all.
BOOST_OPENMETHOD_OVERRIDE(name, (const erased& value), std::string) {
    return te::is_empty(value) ? "nothing" : "something else";
}
// end::method[]

// `int` is registered because `weigh`'s overrider names it; it has no
// `name` overrider, so the catch-all applies to it.

BOOST_OPENMETHOD(weigh, (virtual_<const erased&>), int);

BOOST_OPENMETHOD_OVERRIDE(weigh, (const int& value), int) {
    return value;
}

BOOST_AUTO_TEST_CASE(type_erasure_examples) {
    initialize();

    {
        capture_cout cout;

        // tag::dispatch[]
        const erased spot(Dog{"Spot"});
        const erased answer(42);

        std::cout << name(spot) << "\n"; // Spot the dog

        // `int` is registered - `weigh`'s overrider names it - but has
        // no `name` overrider of its own, so the catch-all applies.
        std::cout << name(answer) << "\n"; // something else
        // end::dispatch[]

        BOOST_TEST(cout.str() == "Spot the dog\nsomething else\n");
    }
}
