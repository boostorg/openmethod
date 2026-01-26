// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include "classes.hpp"

#if !defined(_MSC_VER)
#define METHOD_API boost::openmethod::declspec_none
#elif defined(BOOST_OPENMETHOD_DYNAMIC_LOADING_METHOD_EXPORTS)
#define METHOD_API boost::openmethod::dllexport
#else
#define METHOD_API boost::openmethod::dllimport
#endif

#include <string>

BOOST_OPENMETHOD(
    speak, (boost::openmethod::virtual_ptr<Animal>), std::string, METHOD_API);
