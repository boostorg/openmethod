// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// static_extensions.cpp

// In the static-linking example, the shared library owns the registry state,
// and the executable imports it. This keeps the link dependency in the usual
// direction: on Windows, the executable links against the library's import
// library, never the other way around.
#define BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY
#include "extensions.cpp"
