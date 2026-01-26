// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if !defined(_MSC_VER)
#define REGISTRY_API boost::openmethod::declspec_none
#elif BOOST_OPENMETHOD_DYNAMIC_LOADING_REGISTRY_EXPORTS
#define REGISTRY_API boost::openmethod::dllexport
#else
#define REGISTRY_API boost::openmethod::dllimport
#endif

#include <boost/openmethod/preamble.hpp>

namespace boost::openmethod {
REGISTRY_API boost_openmethod_declspec(default_registry_attributes);
}
