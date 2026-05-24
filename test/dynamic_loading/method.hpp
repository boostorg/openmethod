// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "registry.hpp"
#include "classes.hpp"

#include <string>

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(EXPORT_METHOD)
#define METHOD_DECLSPEC boost::openmethod::dllexport
#else
#define METHOD_DECLSPEC boost::openmethod::dllimport
#endif
#else
#define METHOD_DECLSPEC boost::openmethod::declspec_none
#endif

BOOST_OPENMETHOD(
    speak, (boost::openmethod::virtual_ptr<Animal>), const char*,
    METHOD_DECLSPEC);

inline auto call_speak(boost::openmethod::virtual_ptr<Animal> animal) {
    return speak(animal);
}