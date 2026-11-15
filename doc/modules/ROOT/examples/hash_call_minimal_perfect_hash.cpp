// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// clang-format off

// tag::content[]
#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/minimal_perfect_hash.hpp>

using namespace boost::openmethod;

struct hashed_registry
    : default_registry::with<policies::minimal_perfect_hash<>> {};

auto hash_of(type_id type) -> std::size_t {
    return policies::minimal_perfect_hash<>::fn<hashed_registry>::hash(type);
}
// end::content[]

int main() {
}
