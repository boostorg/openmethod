// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the alternative fix for
// compile_fail_method_takes_erased_by_value.cpp, and compiled as a test.

#include "preamble.hpp"

// tag::content[]
#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/openmethod/interop/boost_type_erasure.hpp>

namespace te = boost::type_erasure;

using any_node_ref = te::any<
    boost::mpl::vector<te::copy_constructible<>, te::relaxed>,
    const te::_self&>;

BOOST_OPENMETHOD(postfix, (virtual_<any_node_ref>, std::ostream&), void);
// end::content[]

void call(any_node_ref node) {
    postfix(node, std::cout);
}
