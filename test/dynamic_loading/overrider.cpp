// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "method.hpp"

#if defined(_WIN32) || defined(__CYGWIN__)
#include <boost/config.hpp>
#else
#include <boost/dll/alias.hpp>
#endif

using namespace boost::openmethod;
namespace mp11 = boost::mp11;

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Dog>), const char*) {
    return "woof";
}

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Dog> a, virtual_ptr<Dog> b), greeting) {
    std::string next_word = has_next() ? next(a, b).first : "n/a";
    return {"wag tails", next_word};
}

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

extern "C" {

BOOST_SYMBOL_EXPORT const void* overrider_get_ids() {
    return get_ids();
}

BOOST_SYMBOL_EXPORT void
overrider_make_dog(unique_virtual_ptr<Animal>& p) {
    p = make_dog();
}

BOOST_SYMBOL_EXPORT const char*
overrider_call_speak(boost::openmethod::virtual_ptr<Animal> animal) {
    return speak(animal);
}

// See method_call_meet: returned via out-parameter to avoid a non-POD return
// type on an extern "C" function (-Wreturn-type-c-linkage).
BOOST_SYMBOL_EXPORT void overrider_call_meet(
    greeting& result, boost::openmethod::virtual_ptr<Animal> a,
    boost::openmethod::virtual_ptr<Animal> b) {
    result = meet(a, b);
}
}
