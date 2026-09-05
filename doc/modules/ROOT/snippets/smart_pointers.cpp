// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/std_shared_ptr.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>
#include <boost/openmethod/interop/std_weak_ptr.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "capture.hpp"

using namespace boost::openmethod;

// tag::classes[]
struct Animal {
    virtual ~Animal() = default;
};
struct Dog : Animal {};
struct Cat : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);
// end::classes[]

namespace by_value {

// tag::shared_by_value[]
BOOST_OPENMETHOD(poke, (virtual_<std::shared_ptr<Animal>>), std::string);

BOOST_OPENMETHOD_OVERRIDE(poke, (std::shared_ptr<Dog> animal), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (std::shared_ptr<Cat> animal), std::string) {
    return "hiss";
}
// end::shared_by_value[]

} // namespace by_value

namespace by_reference {

// tag::shared_by_reference[]
BOOST_OPENMETHOD(poke, (virtual_<const std::shared_ptr<Animal>&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(
    poke, (const std::shared_ptr<Dog>& animal), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(
    poke, (const std::shared_ptr<Cat>& animal), std::string) {
    return "hiss";
}
// end::shared_by_reference[]

} // namespace by_reference

namespace unique {

// tag::unique_by_value[]
BOOST_OPENMETHOD(poke, (virtual_<std::unique_ptr<Animal>>), std::string);

BOOST_OPENMETHOD_OVERRIDE(poke, (std::unique_ptr<Dog> animal), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (std::unique_ptr<Cat> animal), std::string) {
    return "hiss";
}
// end::unique_by_value[]

} // namespace unique

namespace shared_vptr {

BOOST_OPENMETHOD(poke, (shared_virtual_ptr<Animal>), std::string);

BOOST_OPENMETHOD_OVERRIDE(poke, (shared_virtual_ptr<Dog> animal), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (shared_virtual_ptr<Cat> animal), std::string) {
    return "hiss";
}

} // namespace shared_vptr

namespace unique_vptr {

BOOST_OPENMETHOD(poke, (unique_virtual_ptr<Animal>), std::string);

BOOST_OPENMETHOD_OVERRIDE(poke, (unique_virtual_ptr<Dog> animal), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (unique_virtual_ptr<Cat> animal), std::string) {
    return "hiss";
}

} // namespace unique_vptr

BOOST_AUTO_TEST_CASE(shared_ptr_examples) {
    initialize();

    {
        using namespace shared_vptr;
        capture_cout cout;

        // tag::make_shared_virtual[]
        shared_virtual_ptr<Animal> animal = make_shared_virtual<Dog>();

        std::cout << poke(animal) << "\n"; // bark
        // end::make_shared_virtual[]

        BOOST_TEST(cout.str() == "bark\n");
    }

    {
        // tag::shared_virtual_ptr_alias[]
        shared_virtual_ptr<Animal> animal = make_shared_virtual<Dog>();
        std::shared_ptr<Animal> owner = animal.pointer();

        BOOST_TEST(owner.use_count() == 2);
        // end::shared_virtual_ptr_alias[]
    }

    {
        using namespace by_value;
        capture_cout cout;

        // tag::shared_by_value_call[]
        std::cout << poke(std::make_shared<Dog>()) << "\n"; // bark
        std::cout << poke(std::make_shared<Cat>()) << "\n"; // hiss
        // end::shared_by_value_call[]

        BOOST_TEST(cout.str() == "bark\nhiss\n");
    }

    {
        using namespace by_reference;
        capture_cout cout;

        // tag::shared_by_reference_call[]
        const std::shared_ptr<Animal> snoopy = std::make_shared<Dog>();

        std::cout << poke(snoopy) << "\n"; // bark

        BOOST_TEST(snoopy.use_count() == 1);
        // end::shared_by_reference_call[]

        BOOST_TEST(cout.str() == "bark\n");
    }
}

BOOST_AUTO_TEST_CASE(unique_ptr_examples) {
    initialize();

    {
        using namespace unique_vptr;
        capture_cout cout;

        // tag::make_unique_virtual[]
        unique_virtual_ptr<Animal> animal = make_unique_virtual<Dog>();

        std::cout << poke(std::move(animal)) << "\n"; // bark
        // end::make_unique_virtual[]

        BOOST_TEST(cout.str() == "bark\n");
    }

    {
        // tag::unique_virtual_ptr_alias[]
        unique_virtual_ptr<Animal> animal = make_unique_virtual<Dog>();
        unique_virtual_ptr<Animal> owner = std::move(animal);

        BOOST_TEST(owner.get() != nullptr);
        BOOST_TEST(animal.get() == nullptr);
        // end::unique_virtual_ptr_alias[]
    }

    {
        using namespace unique;
        capture_cout cout;

        // tag::unique_by_value_call[]
        std::cout << poke(std::make_unique<Dog>()) << "\n"; // bark
        std::cout << poke(std::make_unique<Cat>()) << "\n"; // hiss
        // end::unique_by_value_call[]

        BOOST_TEST(cout.str() == "bark\nhiss\n");
    }
}

BOOST_AUTO_TEST_CASE(weak_ptr_examples) {
    initialize();

    {
        using namespace shared_vptr;
        capture_cout cout;

        // tag::weak_lock[]
        shared_virtual_ptr<Animal> animal = make_shared_virtual<Dog>();
        weak_virtual_ptr<Animal> observer = animal;

        std::cout << poke(observer.lock()) << "\n"; // bark

        animal = nullptr;
        std::cout << std::boolalpha << observer.expired() << "\n"; // true
        // end::weak_lock[]

        BOOST_TEST(cout.str() == "bark\ntrue\n");
    }

    {
        // tag::weak_virtual_ptr_alias[]
        shared_virtual_ptr<Animal> animal = make_shared_virtual<Dog>();
        weak_virtual_ptr<Animal> observer = animal;
        std::weak_ptr<Animal> weak = observer.pointer();

        BOOST_TEST(animal.pointer().use_count() == 1);
        BOOST_TEST(weak.lock() == animal.pointer());
        // end::weak_virtual_ptr_alias[]
    }
}
