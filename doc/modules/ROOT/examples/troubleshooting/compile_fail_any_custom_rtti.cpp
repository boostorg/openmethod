// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: requires standard RTTI

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/policies/static_rtti.hpp>
#include <boost/openmethod/interop/std_any.hpp>

struct static_registry : default_registry::with<policies::static_rtti> {};

BOOST_OPENMETHOD(
    postfix, (virtual_<const std::any&>, std::ostream&), void, static_registry);
// end::content[]

void call(const std::any& node) {
    postfix(node, std::cout);
}
