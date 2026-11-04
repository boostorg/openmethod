// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: two or more stateful policies in this registry share the same nested

#include "preamble.hpp"

// tag::content[]
struct counter {
    int n = 0;
};

struct calls_policy {
    using category = calls_policy;

    template<class Registry>
    struct fn {
        using state = counter;
    };
};

struct errors_policy {
    using category = errors_policy;

    template<class Registry>
    struct fn {
        using state = counter;
    };
};

struct counting_registry :
    default_registry::with<calls_policy, errors_policy> {};

BOOST_OPENMETHOD_CLASSES(Node, Plus, counting_registry);
// end::content[]
