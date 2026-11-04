// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: do not wrap an object that has a boost_openmethod_vptr overload in a virtual_ptr

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/inplace_vptr.hpp>

struct Term : inplace_vptr_base<Term> {
    virtual ~Term() = default;
};

void print(Term& term) {
    virtual_ptr<Term> p(term);
}
// end::content[]
