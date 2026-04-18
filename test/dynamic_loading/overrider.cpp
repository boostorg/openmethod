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

#ifdef _WIN32
extern "C" {
    BOOST_SYMBOL_EXPORT const void* overrider_get_ids    = (const void*)&get_ids;
    BOOST_SYMBOL_EXPORT const void* overrider_get_fn     = (const void*)&get_fn;
    BOOST_SYMBOL_EXPORT const void* overrider_make_dog   = (const void*)&make_dog;
    BOOST_SYMBOL_EXPORT const void* overrider_call_speak = (const void*)&call_speak;
}
#else
BOOST_DLL_ALIAS(get_ids, overrider_get_ids)
BOOST_DLL_ALIAS(get_fn, overrider_get_fn)
BOOST_DLL_ALIAS(make_dog, overrider_make_dog)
BOOST_DLL_ALIAS(call_speak, overrider_call_speak)
#endif
