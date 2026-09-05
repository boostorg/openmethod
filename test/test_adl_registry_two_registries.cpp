// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Two hierarchies with different affinities in one translation unit. Neither
// method names a registry; each lands in its own, and the two initialize and
// dispatch independently.

#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE adl_registry_two_registries
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry::with<policies::runtime_checks> {};
struct garage_registry : default_registry::with<policies::runtime_checks> {};

namespace zoo {

struct Animal {
    virtual ~Animal() = default;
    friend auto boost_openmethod_registry(Animal*) -> zoo_registry;
};

struct Dog : Animal {};

} // namespace zoo

namespace garage {

struct Vehicle {
    virtual ~Vehicle() = default;
    friend auto boost_openmethod_registry(Vehicle*) -> garage_registry;
};

struct Truck : Vehicle {};

} // namespace garage

BOOST_OPENMETHOD_CLASSES(zoo::Animal, zoo::Dog, zoo_registry);
BOOST_OPENMETHOD_CLASSES(garage::Vehicle, garage::Truck, garage_registry);

BOOST_OPENMETHOD(describe, (virtual_<const zoo::Animal&>), std::string);
BOOST_OPENMETHOD(inspect, (virtual_<const garage::Vehicle&>), std::string);

static_assert(std::is_same_v<
              BOOST_OPENMETHOD_TYPE(
                  describe, (virtual_<const zoo::Animal&>), std::string),
              method<
                  BOOST_OPENMETHOD_ID(describe),
                  std::string(virtual_<const zoo::Animal&>), zoo_registry>>);

static_assert(
    std::is_same_v<
        BOOST_OPENMETHOD_TYPE(
            inspect, (virtual_<const garage::Vehicle&>), std::string),
        method<
            BOOST_OPENMETHOD_ID(inspect),
            std::string(virtual_<const garage::Vehicle&>), garage_registry>>);

BOOST_OPENMETHOD_OVERRIDE(describe, (const zoo::Dog&), std::string) {
    return "a dog";
}

BOOST_OPENMETHOD_OVERRIDE(inspect, (const garage::Truck&), std::string) {
    return "a truck";
}

BOOST_AUTO_TEST_CASE(two_registries_side_by_side) {
    initialize<zoo_registry>();
    initialize<garage_registry>();

    zoo::Dog rex;
    garage::Truck lorry;

    BOOST_TEST(describe(rex) == "a dog");
    BOOST_TEST(inspect(lorry) == "a truck");
}
