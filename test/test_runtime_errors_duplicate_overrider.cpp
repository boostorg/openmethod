// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod/default_registry.hpp>

#include "test_capture_errors.hpp"

struct test_registry
    : boost::openmethod::default_registry::with<capture_output> {};

#define BOOST_OPENMETHOD_DEFAULT_REGISTRY test_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE runtime_errors_duplicate_overrider
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

using capture = capture_errors<test_registry>;

struct Animal {
    virtual ~Animal() {
    }
};

struct Dog : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), const char*);

// Two distinct functions with the exact same (virtual) parameter signature,
// registered directly via method<...>::override<Fn> - the primitive that
// BOOST_OPENMETHOD_OVERRIDE itself expands to (see macros.hpp). The macro
// can't express this scenario in a single translation unit (it declares one
// explicit specialization per signature, so writing it twice is a redefinition
// error), but nothing stops two genuinely different overriders - each a
// distinct `Fn` non-type template argument - from sharing a signature this
// way. This is exactly the shape augment_methods()'s overrider consolidation
// must not silently collapse: two overriders with identical virtual parameter
// types for the same method are ambiguous and must be reported as such, not
// resolved by picking one.
auto poke_bark(virtual_ptr<Dog>) -> const char* {
    return "bark";
}

auto poke_woof(virtual_ptr<Dog>) -> const char* {
    return "woof";
}

using poke_method = BOOST_OPENMETHOD_TYPE(poke, (virtual_ptr<Animal>), const char*);
static poke_method::override<poke_bark> poke_bark_registrar;
static poke_method::override<poke_woof> poke_woof_registrar;

BOOST_AUTO_TEST_CASE(duplicate_overrider_is_ambiguous) {
    auto report = initialize().report;
    BOOST_TEST(report.ambiguous == 1u);

    capture capture;
    Dog dog;
    BOOST_CHECK_THROW(poke(dog), ambiguous_call);
    BOOST_TEST(capture().find("ambiguous") != std::string::npos);
}
