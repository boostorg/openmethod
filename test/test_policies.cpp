// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <type_traits>

#define BOOST_TEST_MODULE policies
#include <boost/test/unit_test.hpp>

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/minimal_cover_hash.hpp>
#include <boost/openmethod/policies/minimal_perfect_hash.hpp>
#include <boost/openmethod/policies/two_level_hash.hpp>

#include "test_util.hpp"

using namespace boost::openmethod;
using namespace boost::openmethod::detail;
namespace mp11 = boost::mp11;

using namespace policies;

struct base {
    using category = base;
};

struct derived : base {
    template<class>
    struct fn {};
};

static_assert(std::is_same_v<
              registry<derived>::policy<base>, derived::fn<registry<derived>>>);

static_assert(detail::is_registry<default_registry>);

struct not_a_policy {};
static_assert(!detail::is_registry<not_a_policy>);

using registry1 = test_registry_<__COUNTER__>;
using registry2 = test_registry_<__COUNTER__>;

struct foo {
    using category = foo;
};

struct foo1 : foo {};
struct foo2 : foo {};

struct bar {
    using category = bar;
};

struct bar1 : bar {};
struct bar2 : bar {};

static_assert(std::is_same_v<registry<>::with<foo1>, registry<foo1>>);
static_assert(std::is_same_v<registry<foo1>::with<foo2>, registry<foo2>>);
static_assert(std::is_same_v<
              registry<foo1, bar1>::with<foo2, bar2>, registry<foo2, bar2>>);
static_assert(std::is_same_v<
              registry<foo1, bar1>::with<bar2, foo2>, registry<foo2, bar2>>);
static_assert(
    std::is_same_v<registry<foo1, bar1>::without<bar>, registry<foo1>>);

BOOST_AUTO_TEST_CASE(test_registry) {
    using namespace policies;

    // BOOST_TEST(&registry2::methods != &registry1::methods);
    // BOOST_TEST(&registry2::classes != &registry1::classes);
    BOOST_TEST(&registry2::static_vptr<void> != &registry1::static_vptr<void>);
    // BOOST_TEST(&registry2::dispatch_data != &registry1::dispatch_data);
}

static_assert(has_initialize<
              vptr_vector::fn<registry1>, registry1::compiler<std::tuple<>>,
              std::tuple<>>);
static_assert(!has_initialize<
              std_rtti::fn<registry1>, registry1::compiler<std::tuple<>>,
              std::tuple<>>);
static_assert(has_initialize<
              fast_perfect_hash::fn<registry1>,
              registry1::compiler<std::tuple<>>, std::tuple<>>);

// The alternative `type_hash` policies conform to the same blueprint. Each is a
// class template, so name a specialization; the defaults are what a user who
// does not tune them gets.
static_assert(has_initialize<
              minimal_perfect_hash<>::fn<registry1>,
              registry1::compiler<std::tuple<>>, std::tuple<>>);
static_assert(has_initialize<
              two_level_hash<>::fn<registry1>,
              registry1::compiler<std::tuple<>>, std::tuple<>>);
#if BOOST_OPENMETHOD_HAS_PEXT
static_assert(has_initialize<
              minimal_cover_hash<>::fn<registry1>,
              registry1::compiler<std::tuple<>>, std::tuple<>>);
#endif

// All four are interchangeable: each derives from the `type_hash` category, so
// `with` replaces whichever one a registry already has, in place, rather than
// appending a second - which would leave `vptr_vector` reading the wrong state.
static_assert(std::is_base_of_v<type_hash, fast_perfect_hash>);
static_assert(std::is_base_of_v<type_hash, minimal_perfect_hash<>>);
static_assert(std::is_base_of_v<type_hash, two_level_hash<>>);
static_assert(std::is_base_of_v<type_hash, minimal_cover_hash<>>);
static_assert(std::is_same_v<
              default_registry::with<minimal_perfect_hash<>>::policy<type_hash>,
              minimal_perfect_hash<>::fn<
                  default_registry::with<minimal_perfect_hash<>>>>);
static_assert(
    mp11::mp_size<default_registry::policy_list>::value ==
    mp11::mp_size<
        default_registry::with<minimal_perfect_hash<>>::policy_list>::value);
