// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// The recommended spelling is a hidden friend: it is part of the class, so no
// use of the class can precede it, and the declaration-order rule cannot be
// broken. Argument-dependent lookup finds it from anywhere.

#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE adl_registry_hidden_friend
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry::with<policies::runtime_checks> {};

namespace zoo {

class Animal {
  public:
    virtual ~Animal() = default;

  private:
    friend auto boost_openmethod_registry(Animal*) -> zoo_registry;
};

class Dog : public Animal {};

} // namespace zoo

static_assert(std::is_same_v<registry_affinity<zoo::Animal>, zoo_registry>);
static_assert(std::is_same_v<registry_affinity<zoo::Dog>, zoo_registry>);

// The method lives in a third namespace, and still finds the affinity.
namespace vet {

BOOST_OPENMETHOD_CLASSES(zoo::Animal, zoo::Dog, zoo_registry);

BOOST_OPENMETHOD(examine, (virtual_<const zoo::Animal&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(examine, (const zoo::Dog&), std::string) {
    return "healthy dog";
}

} // namespace vet

BOOST_AUTO_TEST_CASE(a_hidden_friend_cannot_be_declared_late) {
    initialize<zoo_registry>();

    zoo::Dog rex;
    BOOST_TEST(vet::examine(rex) == "healthy dog");
}
