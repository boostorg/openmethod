// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_SHARED_OVERRIDER_HPP
#define BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_SHARED_OVERRIDER_HPP

#include "method.hpp"

// Included identically by method.cpp and overrider.cpp, two modules that
// share the registry state. Without deduping overriders by logical identity
// (see augment_methods() in initialize.hpp), the two textually-identical
// registrations below would be registered as distinct overrider_info objects
// - one per module - and the Cat dispatch slot would be marked ambiguous.
// See the Cat checks in main.cpp's test_shared_state.
BOOST_OPENMETHOD_OVERRIDE(
    speak, (boost::openmethod::virtual_ptr<Cat>), const char*) {
    return "meow";
}

#endif
