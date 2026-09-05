// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// An affinity declared for a base class reaches its derived classes, because
// the derived-to-base pointer conversion makes the base's overload viable. A
// class whose base overload cannot be used - inaccessible or ambiguous - is
// diagnosed rather than defaulted: see compile_fail_adl_registry_*.cpp.

#include <boost/openmethod.hpp>

#define BOOST_TEST_MODULE adl_registry_inheritance
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry::with<policies::runtime_checks> {};
struct kennel_registry : default_registry::with<policies::runtime_checks> {};

namespace zoo {

struct Animal {
    virtual ~Animal() = default;
};

auto boost_openmethod_registry(Animal*) -> zoo_registry;

struct Cat : Animal {};
struct Persian : Cat {};

// An overload on the class itself is an exact match, and beats the base's.
struct Dog : Animal {};
auto boost_openmethod_registry(Dog*) -> kennel_registry;
struct Poodle : Dog {};

// A virtual base is unambiguous, so it still reaches the affinity.
struct Left : virtual Animal {};
struct Right : virtual Animal {};
struct Chimera : Left, Right {};

} // namespace zoo

using namespace zoo;

// inherited, however deep
static_assert(std::is_same_v<registry_affinity<Cat>, zoo_registry>);
static_assert(std::is_same_v<registry_affinity<Persian>, zoo_registry>);

// the exact match wins, and is itself inherited
static_assert(std::is_same_v<registry_affinity<Dog>, kennel_registry>);
static_assert(std::is_same_v<registry_affinity<Poodle>, kennel_registry>);

// one Animal, so one affinity
static_assert(std::is_same_v<registry_affinity<Chimera>, zoo_registry>);

// An unrelated class is untouched by any of it.
struct Widget {
    virtual ~Widget() = default;
};

static_assert(std::is_same_v<registry_affinity<Widget>, default_registry>);

BOOST_AUTO_TEST_CASE(inheritance_is_compile_time_only) {
    BOOST_TEST(true);
}
