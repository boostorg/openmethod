// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_inplace_vptr_derived_registry.cpp,
// and compiled as a test.

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/inplace_vptr.hpp>

struct Term : inplace_vptr_base<Term, my_registry> {};

struct Sum : Term, inplace_vptr_derived<Sum, Term> {};
// end::content[]
