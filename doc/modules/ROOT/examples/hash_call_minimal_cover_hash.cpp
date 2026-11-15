// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// clang-format off

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/minimal_cover_hash.hpp>

using namespace boost::openmethod;

#if BOOST_OPENMETHOD_HAS_PEXT

// tag::content[]
struct hashed_registry
    : default_registry::with<policies::minimal_cover_hash<>> {};

auto hash_of(type_id type) -> std::size_t {
    return policies::minimal_cover_hash<>::fn<hashed_registry>::hash(type);
}
// end::content[]

#endif

int main() {
}
