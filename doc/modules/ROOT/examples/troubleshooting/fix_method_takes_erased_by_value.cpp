// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for
// compile_fail_method_takes_erased_by_value.cpp, and compiled as a test.

#include "preamble.hpp"

// tag::content[]
#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/openmethod/interop/boost_type_erasure.hpp>

namespace te = boost::type_erasure;

using any_node =
    te::any<boost::mpl::vector<te::copy_constructible<>, te::relaxed>>;

BOOST_OPENMETHOD(postfix, (virtual_<const any_node&>, std::ostream&), void);
// end::content[]

void call(const any_node& node) {
    postfix(node, std::cout);
}
