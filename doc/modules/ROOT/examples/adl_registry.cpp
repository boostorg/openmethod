// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE adl_registry
#include <boost/test/unit_test.hpp>

// tag::registry[]
struct zoo_registry :
    boost::openmethod::default_registry::with<
        boost::openmethod::policies::runtime_checks> {};
// end::registry[]

using namespace boost::openmethod;

// tag::affinity[]
namespace zoo {

class Animal {
  public:
    virtual ~Animal() = default;

  private:
    // Animal - and every class derived from it - belongs to zoo_registry
    friend auto boost_openmethod_registry(Animal*) -> zoo_registry;
};

class Dog : public Animal {};
class Cat : public Animal {};

} // namespace zoo
// end::affinity[]

// tag::methods[]
BOOST_OPENMETHOD_CLASSES(zoo::Animal, zoo::Dog, zoo::Cat, zoo_registry);

// no registry argument: speak follows Animal
BOOST_OPENMETHOD(speak, (virtual_<const zoo::Animal&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(speak, (const zoo::Dog&), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (const zoo::Cat&), std::string) {
    return "meow";
}
// end::methods[]

// tag::virtual_ptr[]
// ...and so does virtual_ptr
static_assert(
    std::is_same_v<virtual_ptr<zoo::Dog>, virtual_ptr<zoo::Dog, zoo_registry>>);
// end::virtual_ptr[]

BOOST_AUTO_TEST_CASE(adl_registry) {
    // tag::call[]
    initialize<zoo_registry>();

    zoo::Dog spot;
    zoo::Cat felix;
    // end::call[]

    BOOST_TEST(speak(spot) == "bark");
    BOOST_TEST(speak(felix) == "meow");
}
