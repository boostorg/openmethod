// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Deliberately advance __COUNTER__ before the include, so that a _MEM macro
// naming its members with it would produce a Zoo, and a Zoo::poke, different
// from the ones other_tu.cpp sees. That compiles and links clean; the method
// then dispatches through a different method<> object than the one the
// overriders registered with, and the call reports "not implemented".
enum { advance_the_counter = __COUNTER__ };

#include "member_method.hpp"

#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE cross_tu_member_method
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(member_method_across_translation_units) {
    initialize();

    Dog snoopy;
    Cat felix;

    BOOST_TEST(Zoo::poke(snoopy) == "bark");
    BOOST_TEST(Zoo::poke(felix) == "hiss");
}
