// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Probe for BMI2's parallel bit extract, compiled with -mbmi2. See ../Jamfile,
// and boost/openmethod/policies/minimal_cover_hash.hpp, which is the only part
// of the library that needs the instruction.
//
// It tests the header's own feature macro rather than the intrinsic directly:
// what the test suite needs to know is whether that header will let the policy
// be used, which is a slightly narrower question than whether some spelling of
// pext compiles.

#include <boost/openmethod/policies/minimal_cover_hash.hpp>

#include <cstdint>

static_assert(BOOST_OPENMETHOD_HAS_PEXT);

auto probe(std::uint64_t value, std::uint64_t mask) -> std::uint64_t {
    return boost::openmethod::detail::pext64(value, mask);
}
