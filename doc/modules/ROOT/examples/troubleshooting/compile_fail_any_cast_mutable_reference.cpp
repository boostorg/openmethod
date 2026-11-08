// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// No static_assert: the `cast` that would make the reference is removed from
// the overload set. gcc and clang say "no matching function for call to", MSVC
// "no matching overloaded function found".
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: no matching

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/interop/boost_any.hpp>

BOOST_OPENMETHOD(postfix, (virtual_<const boost::any&>, std::ostream&), void);

BOOST_OPENMETHOD_OVERRIDE(postfix, (Plus&, std::ostream& os), void) {
    os << "+";
}
// end::content[]
