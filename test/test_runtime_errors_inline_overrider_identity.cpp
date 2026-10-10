// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "test_capture_errors.hpp"

#include <boost/openmethod/initialize.hpp>

#include "test_classes.hpp"

#define BOOST_TEST_MODULE runtime_errors_inline_overrider_identity
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

// Two genuinely different overriders of one method, sharing a signature, each
// declared `inline`. augment_methods() merges the copies of an inline
// overrider that several modules registered, and these are the shape that
// merge keys on: same function type, same virtual parameter types, both
// flagged inline_. They are not the same overrider, so they must stay
// ambiguous - overrider_info::identity is what tells the two cases apart.
// Before it existed, this program printed "a" and reported no error.
namespace a {
BOOST_OPENMETHOD_INLINE_OVERRIDE(poke, (virtual_ptr<Dog>), const char*) {
    return "a";
}
} // namespace a

namespace b {
BOOST_OPENMETHOD_INLINE_OVERRIDE(poke, (virtual_ptr<Dog>), const char*) {
    return "b";
}
} // namespace b

BOOST_AUTO_TEST_CASE(distinct_inline_overriders_are_ambiguous) {
    auto report = initialize().report;
    BOOST_TEST(report.ambiguous == 1u);

    capture capture;
    Dog dog;
    BOOST_CHECK_THROW(poke(dog), ambiguous_call);
    BOOST_TEST(capture().find("ambiguous") != std::string::npos);
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
