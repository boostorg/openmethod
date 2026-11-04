// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// The second translation unit of errors_odr_violation.cpp: it flips
// BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS, so `default_registry` has another
// definition here than there.

#ifdef BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
#undef BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
#else
#define BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
#endif

#include <boost/openmethod.hpp>
