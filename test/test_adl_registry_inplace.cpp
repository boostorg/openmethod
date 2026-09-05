// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// `inplace_vptr_base` declares the affinity itself, as a hidden friend. Since
// the hook is now the library's own, a method over such a class needs neither a
// registry argument nor a BOOST_OPENMETHOD_DEFAULT_REGISTRY override.

#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/inplace_vptr.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE adl_registry_inplace
#include <boost/test/unit_test.hpp>

namespace bom = boost::openmethod;

// An inplace_vptr hierarchy needs neither a vptr policy nor a type hash.
struct zoo_registry :
    bom::default_registry::without<
        bom::policies::vptr, bom::policies::type_hash> {};

struct Animal : bom::inplace_vptr_base<Animal, zoo_registry> {};
struct Dog : Animal, bom::inplace_vptr_derived<Dog, Animal> {};
struct Cat : Animal, bom::inplace_vptr_derived<Cat, Animal> {};

// The mixin's hidden friend is what `registry_affinity` reads back.
static_assert(std::is_same_v<bom::registry_affinity<Animal>, zoo_registry>);
static_assert(std::is_same_v<bom::registry_affinity<Dog>, zoo_registry>);

// No registry named, and no #define: the method follows the class.
BOOST_OPENMETHOD(speak, (bom::virtual_<const Animal&>), std::string);

static_assert(std::is_same_v<
              BOOST_OPENMETHOD_TYPE(
                  speak, (bom::virtual_<const Animal&>), std::string),
              bom::method<
                  BOOST_OPENMETHOD_ID(speak),
                  std::string(bom::virtual_<const Animal&>), zoo_registry>>);

BOOST_OPENMETHOD_OVERRIDE(speak, (const Dog&), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (const Cat&), std::string) {
    return "meow";
}

BOOST_AUTO_TEST_CASE(inplace_vptr_supplies_the_affinity) {
    bom::initialize<zoo_registry>();

    Dog spot;
    Cat felix;

    BOOST_TEST(speak(spot) == "bark");
    BOOST_TEST(speak(felix) == "meow");
}
