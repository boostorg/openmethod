// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// A class declares an affinity for a registry once, next to itself. Everything
// that mentions the class then finds that registry on its own: `virtual_ptr`,
// and the methods that take the class as a virtual parameter.

#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE adl_registry
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry::with<policies::runtime_checks> {};

namespace zoo {

struct Animal {
    virtual ~Animal() = default;
};

auto boost_openmethod_registry(Animal*) -> zoo_registry;

struct Dog : Animal {};
struct Cat : Animal {};

} // namespace zoo

// A class that never declared one keeps the default registry.
struct Widget {
    virtual ~Widget() = default;
};

using zoo::Animal, zoo::Dog, zoo::Cat;

static_assert(std::is_same_v<registry_affinity<Animal>, zoo_registry>);
static_assert(std::is_same_v<registry_affinity<Dog>, zoo_registry>);
static_assert(std::is_same_v<registry_affinity<Widget>, default_registry>);

// `virtual_ptr` picks it up, so `virtual_ptr<Dog>` is not a `virtual_ptr` in
// the default registry.
static_assert(std::is_same_v<virtual_ptr<Dog>, virtual_ptr<Dog, zoo_registry>>);
static_assert(
    std::is_same_v<virtual_ptr<Widget>, virtual_ptr<Widget, default_registry>>);

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat, zoo_registry);

// Neither declaration names a registry; both land in `zoo_registry`.
BOOST_OPENMETHOD(speak, (virtual_<const Animal&>), std::string);
BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), std::string);

static_assert(
    std::is_same_v<
        BOOST_OPENMETHOD_TYPE(speak, (virtual_<const Animal&>), std::string),
        method<
            BOOST_OPENMETHOD_ID(speak), std::string(virtual_<const Animal&>),
            zoo_registry>>);

BOOST_OPENMETHOD_OVERRIDE(speak, (const Dog&), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (const Cat&), std::string) {
    return "meow";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog>), std::string) {
    return "woof";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Cat>), std::string) {
    return "hiss";
}

BOOST_AUTO_TEST_CASE(dispatch_in_the_registry_the_class_names) {
    initialize<zoo_registry>();

    Dog spot;
    Cat felix;

    BOOST_TEST(speak(spot) == "bark");
    BOOST_TEST(speak(felix) == "meow");
    BOOST_TEST(poke(virtual_ptr<Dog>(spot)) == "woof");
    BOOST_TEST(poke(virtual_ptr<Cat>(felix)) == "hiss");
}
