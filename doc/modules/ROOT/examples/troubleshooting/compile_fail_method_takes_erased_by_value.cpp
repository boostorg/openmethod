// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page, and checked like test/compile_fail_*.cpp.
// Expected diagnostic, as a CMake regex (see test/CMakeLists.txt).
// expected-error: an owning type_erasure::any must be passed by reference

#include "preamble.hpp"

// tag::content[]
#include <boost/mpl/vector.hpp>
#include <boost/type_erasure/any.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/openmethod/interop/boost_type_erasure.hpp>

namespace te = boost::type_erasure;

using any_node =
    te::any<boost::mpl::vector<te::copy_constructible<>, te::relaxed>>;

BOOST_OPENMETHOD(postfix, (virtual_<any_node>, std::ostream&), void);
// end::content[]

void call(const any_node& node) {
    postfix(node, std::cout);
}
