// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "registry.hpp"
#include "method.hpp"
#include "shared_overrider.hpp"

#include <boost/openmethod.hpp>

#include <iostream>

#if defined(_WIN32) || defined(__CYGWIN__)
#include <boost/config.hpp>
#else
#include <boost/dll/alias.hpp>
#endif

using namespace boost::openmethod;
namespace mp11 = boost::mp11;

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Animal>), const char*) {
    return "?";
}

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Animal>, virtual_ptr<Animal>), greeting) {
    // Base overrider: no next, so the next word is always "n/a".
    return {"ignore", "n/a"};
}

extern "C" {

BOOST_SYMBOL_EXPORT const void* method_state_id() {
    return registry_state_id();
}

BOOST_SYMBOL_EXPORT void method_make_dog(unique_virtual_ptr<Animal>& p) {
    p = make_dog();
}

BOOST_SYMBOL_EXPORT const char*
method_call_speak(boost::openmethod::virtual_ptr<Animal> animal) {
    return speak(animal);
}

// Returned through an out-parameter rather than by value: an extern "C"
// function returning a non-POD type (greeting) trips -Wreturn-type-c-linkage,
// same as method_make_dog above.
BOOST_SYMBOL_EXPORT void method_call_meet(
    greeting& result, boost::openmethod::virtual_ptr<Animal> a,
    boost::openmethod::virtual_ptr<Animal> b) {
    result = meet(a, b);
}
}
