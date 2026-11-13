// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for
// compile_fail_policy_order.cpp, and compiled as a test.

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/policies/minimal_perfect_hash.hpp>

struct hashed_registry :
    default_registry::with<policies::minimal_perfect_hash<>> {};

BOOST_OPENMETHOD_CLASSES(Node, Plus, hashed_registry);
// end::content[]
