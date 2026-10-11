// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Methods whose virtual parameters mix registries that defer their type ids
// with registries that do not. The animals get their type numbers at run time,
// at the start of the test: a type id read during static construction is zero,
// and dispatch goes wrong.

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>
#include <boost/openmethod/initialize.hpp>

#include <cstddef>
#include <string>
#include <type_traits>

#define BOOST_TEST_MODULE mixed_registries_deferred
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

namespace {

auto next_non_polymorphic_id() -> std::size_t {
    static std::size_t counter = 0;

    return (std::size_t(1) << (sizeof(std::size_t) * 8 - 1)) | ++counter;
}

template<typename T>
auto non_polymorphic_static_type() -> std::size_t {
    static std::size_t value = next_non_polymorphic_id();

    return value;
}

struct Animal {
    static std::size_t static_type;
    std::size_t type;

    Animal() : type(static_type) {
    }

  protected:
    explicit Animal(std::size_t type) : type(type) {
    }
};

struct Dog : Animal {
    static std::size_t static_type;

    Dog() : Animal(static_type) {
    }
};

struct Cat : Animal {
    static std::size_t static_type;

    Cat() : Animal(static_type) {
    }
};

std::size_t Animal::static_type;
std::size_t Dog::static_type;
std::size_t Cat::static_type;

void assign_type_ids() {
    Animal::static_type = 1;
    Dog::static_type = 2;
    Cat::static_type = 3;
}

struct late_rtti : policies::deferred_static_rtti {
    template<class Registry>
    struct fn : defaults {
        template<class T>
        static constexpr bool is_polymorphic = std::is_base_of_v<Animal, T>;

        template<typename T>
        static auto static_type() -> type_id {
            if constexpr (is_polymorphic<T>) {
                return type_id(T::static_type);
            } else {
                return type_id(non_polymorphic_static_type<T>());
            }
        }

        template<typename T>
        static auto dynamic_type(const T& obj) -> type_id {
            if constexpr (is_polymorphic<T>) {
                return type_id(obj.type);
            } else {
                return type_id(non_polymorphic_static_type<T>());
            }
        }

        static auto type_index(type_id type) -> type_id {
            return type;
        }
    };
};

struct Vehicle {
    virtual ~Vehicle() = default;
};

struct Car : Vehicle {};
struct Truck : Vehicle {};

struct Planet {
    virtual ~Planet() = default;
};

struct Earth : Planet {};
struct Mars : Planet {};

struct late_registry :
    default_registry::with<late_rtti>::without<policies::type_hash>::with<
        policies::throw_error_handler> {};

struct garage_registry :
    default_registry::with<
        policies::runtime_checks, policies::throw_error_handler> {};

struct space_registry :
    default_registry::with<policies::throw_error_handler> {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat, late_registry);
BOOST_OPENMETHOD_CLASSES(Vehicle, Car, Truck, garage_registry);
BOOST_OPENMETHOD_CLASSES(Planet, Earth, Mars, space_registry);

// The method's registry defers; so does its only parameter's.
BOOST_OPENMETHOD(sound, (virtual_<const Animal&>), std::string, late_registry);

BOOST_OPENMETHOD_OVERRIDE(sound, (const Dog&), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(sound, (const Cat&), std::string) {
    return "meow";
}

// The method's registry does not defer; one parameter's does, the first or the
// second.
BOOST_OPENMETHOD(
    ride, (virtual_<const Animal&, late_registry>, virtual_<const Vehicle&>),
    std::string, garage_registry);

BOOST_OPENMETHOD_OVERRIDE(ride, (const Dog&, const Car&), std::string) {
    return "dog, car";
}

BOOST_OPENMETHOD_OVERRIDE(ride, (const Cat&, const Truck&), std::string) {
    return "cat, truck";
}

BOOST_OPENMETHOD(
    drive, (virtual_<const Vehicle&>, virtual_<const Animal&, late_registry>),
    std::string, garage_registry);

BOOST_OPENMETHOD_OVERRIDE(drive, (const Car&, const Cat&), std::string) {
    return "car, cat";
}

BOOST_OPENMETHOD_OVERRIDE(drive, (const Truck&, const Dog&), std::string) {
    return "truck, dog";
}

// The method's registry defers; its foreign parameter's does not.
BOOST_OPENMETHOD(
    visit, (virtual_<const Animal&>, virtual_<const Planet&, space_registry>),
    std::string, late_registry);

BOOST_OPENMETHOD_OVERRIDE(visit, (const Dog&, const Earth&), std::string) {
    return "dog, earth";
}

BOOST_OPENMETHOD_OVERRIDE(visit, (const Cat&, const Mars&), std::string) {
    return "cat, mars";
}

} // namespace

BOOST_AUTO_TEST_CASE(mixed_deferral) {
    assign_type_ids();

    initialize<space_registry>();
    initialize<late_registry>();
    initialize<garage_registry>();

    Dog dog;
    Cat cat;
    Car car;
    Truck truck;
    Earth earth;
    Mars mars;

    BOOST_TEST(sound(dog) == "bark");
    BOOST_TEST(sound(cat) == "meow");

    BOOST_TEST(ride(dog, car) == "dog, car");
    BOOST_TEST(ride(cat, truck) == "cat, truck");

    BOOST_TEST(drive(car, cat) == "car, cat");
    BOOST_TEST(drive(truck, dog) == "truck, dog");

    BOOST_TEST(visit(dog, earth) == "dog, earth");
    BOOST_TEST(visit(cat, mars) == "cat, mars");
}
