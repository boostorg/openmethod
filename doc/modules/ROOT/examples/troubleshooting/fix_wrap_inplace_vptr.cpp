// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_wrap_inplace_vptr.cpp,
// and compiled as a test.

#include "preamble.hpp"

// tag::content[]
#include <boost/openmethod/inplace_vptr.hpp>

struct Term : inplace_vptr_base<Term> {
    virtual ~Term() = default;
};

BOOST_OPENMETHOD(postfix, (virtual_<Term&>, std::ostream&), void);

void print(Term& term) {
    postfix(term, std::cout);
}
// end::content[]
