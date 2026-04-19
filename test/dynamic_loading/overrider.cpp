// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "method.hpp"

#ifdef _WIN32
#include <boost/config.hpp>
#else
#include <boost/dll/alias.hpp>
#endif

using namespace boost::openmethod;
namespace mp11 = boost::mp11;

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Dog>), const char*) {
    return "woof";
}

BOOST_OPENMETHOD_CLASSES(Animal, Dog);


#if defined(_MSC_VER)
#pragma warning(disable : 4190) // C-linkage function returns UDT
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
#endif

extern "C" {

BOOST_SYMBOL_EXPORT const void* overrider_get_ids() {
    return get_ids();
}

BOOST_SYMBOL_EXPORT const void* overrider_get_fn() {
    return get_fn();
}

BOOST_SYMBOL_EXPORT void
overrider_make_dog(unique_virtual_ptr<Animal>& p) {
    p = make_dog();
}

BOOST_SYMBOL_EXPORT const char*
overrider_call_speak(boost::openmethod::virtual_ptr<Animal> animal) {
    return speak(animal);
}
}
