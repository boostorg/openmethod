// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_SHARED_OVERRIDER_HPP
#define BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_SHARED_OVERRIDER_HPP

#include "method.hpp"

// Included identically by method.cpp and overrider.cpp, two modules that
// share the registry state. Must be declared INLINE: BOOST_OPENMETHOD_OVERRIDE
// would be an ODR violation here (a non-inline function defined identically
// in two translation units), and augment_methods()'s cross-module dedup (see
// initialize.hpp) only ever merges overrider_info entries marked inline_ -
// exactly because only `inline` guarantees the same definition may legally
// appear in more than one module. Without BOOST_OPENMETHOD_INLINE_OVERRIDE,
// the two textually-identical registrations below would be registered as
// distinct, non-mergeable overrider_info objects - one per module - and the
// Cat dispatch slot would be marked ambiguous. See the Cat checks in
// main.cpp's test_shared_state.
BOOST_OPENMETHOD_INLINE_OVERRIDE(
    speak, (boost::openmethod::virtual_ptr<Cat>), const char*) {
    return "meow";
}

#endif
