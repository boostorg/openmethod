// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// How a method picks its registry: it takes the affinity its virtual
// parameters agree on. Only a *declared* affinity constrains it, so a class
// with the default affinity yields rather than conflicting. A registry named on
// the declaration wins outright, and no scan happens.

#include <string>

#include <boost/openmethod.hpp>

#define BOOST_TEST_MODULE adl_registry_scan
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry::with<policies::runtime_checks> {};
struct other_registry : default_registry::with<policies::runtime_checks> {};

namespace zoo {

struct Animal {
    virtual ~Animal() = default;
};

auto boost_openmethod_registry(Animal*) -> zoo_registry;

struct Dog : Animal {};

} // namespace zoo

struct Widget {
    virtual ~Widget() = default;
};

using zoo::Animal, zoo::Dog;

template<typename Fn>
using scan = detail::method_registry<Fn>;

// One virtual parameter with an affinity, in either shape.
static_assert(
    std::is_same_v<scan<void(virtual_<const Animal&>)>, zoo_registry>);
static_assert(std::is_same_v<scan<void(virtual_ptr<Animal>)>, zoo_registry>);
static_assert(std::is_same_v<scan<void(virtual_ptr<Animal>&)>, zoo_registry>);
static_assert(
    std::is_same_v<scan<void(const virtual_ptr<Animal>&)>, zoo_registry>);

// The affinity is inherited, so a derived class carries it too.
static_assert(std::is_same_v<scan<void(virtual_<const Dog&>)>, zoo_registry>);

// No affinity anywhere: the macro default, exactly as before this feature.
static_assert(
    std::is_same_v<scan<void(virtual_<const Widget&>)>, default_registry>);
static_assert(std::is_same_v<scan<void()>, default_registry>);

// Mixing a class that declares an affinity with one that does not: the latter
// yields. This is what keeps a first affinity from cascading errors.
static_assert(std::is_same_v<
              scan<void(virtual_<const Animal&>, virtual_<const Widget&>)>,
              zoo_registry>);
static_assert(std::is_same_v<
              scan<void(virtual_<const Widget&>, virtual_<const Animal&>)>,
              zoo_registry>);

// Mixed shapes agreeing.
static_assert(
    std::is_same_v<
        scan<void(virtual_ptr<Animal>, virtual_<const Dog&>)>, zoo_registry>);

// Non-virtual parameters are ignored.
static_assert(std::is_same_v<
              scan<void(int, virtual_<const Animal&>, char*)>, zoo_registry>);

// A registry named on the declaration wins, and the parameters are not
// consulted at all - the form that predates this feature.
BOOST_OPENMETHOD(ping, (virtual_<const Widget&>), std::string, other_registry);

static_assert(std::is_same_v<
              BOOST_OPENMETHOD_TYPE(
                  ping, (virtual_<const Widget&>), std::string, other_registry),
              method<
                  BOOST_OPENMETHOD_ID(ping),
                  std::string(virtual_<const Widget&>), other_registry>>);

BOOST_AUTO_TEST_CASE(scan_is_compile_time_only) {
    BOOST_TEST(true);
}
