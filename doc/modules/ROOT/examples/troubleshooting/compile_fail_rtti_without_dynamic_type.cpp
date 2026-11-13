// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: the rtti policy deems this class polymorphic, so it must provide dynamic_type

#include <type_traits>
#include <typeinfo>

#include "preamble.hpp"

// tag::content[]
struct my_rtti : policies::rtti {
    template<class Registry>
    struct fn : defaults {
        template<class T>
        static constexpr bool is_polymorphic = std::is_polymorphic_v<T>;

        template<class T>
        static auto static_type() -> type_id {
            return &typeid(T);
        }
    };
};

struct rtti_registry : default_registry::with<my_rtti> {};

void print(Node& node) {
    virtual_ptr<Node, rtti_registry> p(node);
}
// end::content[]
