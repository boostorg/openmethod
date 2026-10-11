// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// A method may be declared in any registry, whatever registries its parameters
// carry - through a class affinity, or spelled on the parameter. A parameter
// that carries another registry dispatches in it.

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>
#include <boost/openmethod/policies/vptr_map.hpp>
#include <boost/openmethod/initialize.hpp>

#include <string>
#include <type_traits>

#define BOOST_TEST_MODULE mixed_registries_affinity
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry::with<policies::vptr_map<>> {};

// Holds the methods.
struct other_registry :
    default_registry::with<policies::throw_error_handler> {};

struct Animal {
    virtual ~Animal() = default;
    friend auto boost_openmethod_registry(Animal*) -> zoo_registry;
};

struct Dog : Animal {};

// No affinity.
struct Vehicle {
    virtual ~Vehicle() = default;
};

struct Car : Vehicle {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog);
BOOST_OPENMETHOD_CLASSES(Vehicle, Car, zoo_registry);
BOOST_OPENMETHOD_CLASSES(Vehicle, Car, other_registry);

static_assert(std::is_same_v<registry_affinity<Animal>, zoo_registry>);

// `virtual_ptr<Animal>` carries `zoo_registry`, by affinity.
BOOST_OPENMETHOD(by_value, (virtual_ptr<Animal>), std::string, other_registry);

BOOST_OPENMETHOD_OVERRIDE(by_value, (virtual_ptr<Dog>), std::string) {
    return "dog";
}

BOOST_OPENMETHOD(
    by_ref, (const virtual_ptr<Animal>&), std::string, other_registry);

BOOST_OPENMETHOD_OVERRIDE(by_ref, (const virtual_ptr<Dog>&), std::string) {
    return "dog";
}

// `virtual_<const Animal&>` carries it too, by affinity.
BOOST_OPENMETHOD(
    by_virtual, (virtual_<const Animal&>), std::string, other_registry);

BOOST_OPENMETHOD_OVERRIDE(by_virtual, (const Dog&), std::string) {
    return "dog";
}

// `Vehicle` declares nothing: spelled, it goes to `zoo_registry`; otherwise it
// adopts the method's registry.
BOOST_OPENMETHOD(
    spelled, (virtual_<const Vehicle&, zoo_registry>), std::string,
    other_registry);

BOOST_OPENMETHOD_OVERRIDE(spelled, (const Car&), std::string) {
    return "car";
}

BOOST_OPENMETHOD(
    adopted, (virtual_<const Animal&>, virtual_<const Vehicle&>), std::string,
    other_registry);

BOOST_OPENMETHOD_OVERRIDE(adopted, (const Dog&, const Car&), std::string) {
    return "dog, car";
}

BOOST_AUTO_TEST_CASE(any_method_registry) {
    initialize<zoo_registry>();
    initialize<other_registry>();

    Dog dog;
    Car car;

    BOOST_TEST(by_value(virtual_ptr<Animal>(dog)) == "dog");
    BOOST_TEST(by_ref(virtual_ptr<Animal>(dog)) == "dog");
    BOOST_TEST(by_virtual(dog) == "dog");
    BOOST_TEST(spelled(car) == "car");
    BOOST_TEST(adopted(dog, car) == "dog, car");
}
