// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: registry can be specified only for root classes

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/inplace_vptr.hpp>

struct Term : inplace_vptr_base<Term> {};

struct Sum : Term, inplace_vptr_derived<Sum, Term, my_registry> {};
// end::content[]
