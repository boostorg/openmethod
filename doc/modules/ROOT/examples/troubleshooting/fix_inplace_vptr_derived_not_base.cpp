// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Shown on the troubleshooting page as the fix for compile_fail_inplace_vptr_derived_not_base.cpp,
// and compiled as a test.

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

// tag::content[]
#include <boost/openmethod/inplace_vptr.hpp>

struct Node : inplace_vptr_base<Node> {
    virtual ~Node() = default;
};

struct Minus : Node, inplace_vptr_derived<Minus, Node> {};
// end::content[]
