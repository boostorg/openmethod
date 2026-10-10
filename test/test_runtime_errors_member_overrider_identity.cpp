// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_capture_errors.hpp"

#include <boost/openmethod/initialize.hpp>

#include "test_classes.hpp"

#define BOOST_TEST_MODULE runtime_errors_member_overrider_identity
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

using capture = capture_errors<test_registry>;

struct Animal {
    virtual ~Animal() {
    }
};

struct Dog : Animal {};

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog);

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), const char*);

// The same shape as the namespace-scope test beside this one, but reached the
// way a member overrider makes easy: a _MEM overrider's body is in a class, so
// it is implicitly inline and registers as inline_. Two classes may each add
// an overrider of one method with one signature - which the free macros cannot
// express in a single translation unit - and the two are distinct.
class VetA {
    BOOST_OPENMETHOD_OVERRIDE_MEM(poke, (virtual_ptr<Dog>), const char*) {
        return "A";
    }
};

class VetB {
    BOOST_OPENMETHOD_OVERRIDE_MEM(poke, (virtual_ptr<Dog>), const char*) {
        return "B";
    }
};

BOOST_AUTO_TEST_CASE(distinct_member_overriders_are_ambiguous) {
    auto report = initialize().report;
    BOOST_TEST(report.ambiguous == 1u);

    capture capture;
    Dog dog;
    BOOST_CHECK_THROW(poke(dog), ambiguous_call);
    BOOST_TEST(capture().find("ambiguous") != std::string::npos);
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
