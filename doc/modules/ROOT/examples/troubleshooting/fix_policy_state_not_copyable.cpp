// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for
// compile_fail_policy_state_not_copyable.cpp, and compiled as a test.

#include <tuple>

#include "preamble.hpp"

// tag::content[]
#include <memory>
#include <boost/openmethod/initialize.hpp>

struct cache_policy {
    using category = cache_policy;

    template<class Registry>
    struct fn {
        struct state {
            std::shared_ptr<int> data;
        };

        template<class Context, class... Options>
        static void initialize(const Context&, const std::tuple<Options...>&) {
        }
    };
};

struct cached_registry : default_registry::with<cache_policy> {};

BOOST_OPENMETHOD_CLASSES(Node, Plus, cached_registry);

void init() {
    initialize<cached_registry>();
}
// end::content[]
