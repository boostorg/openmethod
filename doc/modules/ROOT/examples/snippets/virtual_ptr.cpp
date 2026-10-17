// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/std_shared_ptr.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "capture.hpp"

using namespace boost::openmethod;

namespace polymorphic_classes {

// tag::polymorphic_classes[]
struct Animal {
    virtual ~Animal() = default;
};
struct Dog : Animal {};
struct Cat : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);
// end::polymorphic_classes[]

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), std::string);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog> animal), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Cat> animal), std::string) {
    return "hiss";
}

} // namespace polymorphic_classes

namespace non_polymorphic_classes {

// tag::non_polymorphic_classes[]
// classes not required to be polymorphic
struct Animal {};
struct Cat : Animal {};
struct Dog : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog);
// end::non_polymorphic_classes[]

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), std::string);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog> animal), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Cat> animal), std::string) {
    return "hiss";
}

} // namespace non_polymorphic_classes

BOOST_AUTO_TEST_CASE(virtual_ptr_examples) {
    // tag::initialize[]
    initialize();
    // end::initialize[]

    {
        using namespace non_polymorphic_classes;
        poke(make_unique_virtual<Dog>()); // for coverage
    }

    {
        using namespace polymorphic_classes;
        // tag::ctor_nullptr[]
        virtual_ptr<Dog> p{nullptr};

        BOOST_TEST(p.get() == nullptr);
        BOOST_TEST(p.vptr() == nullptr);
        // end::ctor_nullptr[]
    }

    {
        using namespace polymorphic_classes;
        // tag::ctor_ref[]
        Dog snoopy;
        Animal& animal = snoopy;

        virtual_ptr<Animal> p = animal;

        BOOST_TEST(p.get() == &snoopy);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::ctor_ref[]
    }

    {
        using namespace polymorphic_classes;
        // tag::ctor_pointer[]
        Dog snoopy;
        Animal* animal = &snoopy;

        virtual_ptr<Animal> p = animal;

        BOOST_TEST(p.get() == &snoopy);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::ctor_pointer[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::ctor_vptr[]
        Dog snoopy;
        virtual_ptr<Dog> dog = final_virtual_ptr(snoopy);

        virtual_ptr<Animal> p = dog;

        BOOST_TEST(p.get() == &snoopy);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::ctor_vptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::ctor_shared_vptr[]
        virtual_ptr<std::shared_ptr<Animal>> snoopy =
            make_shared_virtual<Dog>();
        virtual_ptr<Animal> p = snoopy;

        BOOST_TEST(p.get() == snoopy.get());
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::ctor_shared_vptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::ctor_shared_from_plain_rejected[]
        static_assert(
            std::is_constructible_v<
                shared_virtual_ptr<Animal>, virtual_ptr<Dog>> == false);
        // end::ctor_shared_from_plain_rejected[]
    }

    {
        using namespace polymorphic_classes;
        // tag::assign_ref[]
        virtual_ptr<Animal> p{nullptr};
        Dog snoopy;
        Animal& animal = snoopy;

        p = animal;

        BOOST_TEST(p.get() == &snoopy);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::assign_ref[]
    }

    {
        using namespace polymorphic_classes;
        // tag::assign_pointer[]
        virtual_ptr<Animal> p{nullptr};
        Dog snoopy;
        Animal* animal = &snoopy;

        p = animal;

        BOOST_TEST(p.get() == &snoopy);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::assign_pointer[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::assign_vptr[]
        Dog snoopy;
        virtual_ptr<Dog> dog = final_virtual_ptr(snoopy);
        virtual_ptr<Animal> p{nullptr};

        p = dog;

        BOOST_TEST(p.get() == &snoopy);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::assign_vptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::assign_shared_vptr[]
        virtual_ptr<std::shared_ptr<Animal>> snoopy =
            make_shared_virtual<Dog>();
        virtual_ptr<Animal> p;

        p = snoopy;

        BOOST_TEST(p.get() == snoopy.get());
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::assign_shared_vptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::assign_shared_from_plain_rejected[]
        static_assert(
            std::is_assignable_v<
                shared_virtual_ptr<Animal>&, virtual_ptr<Dog>> == false);
        // end::assign_shared_from_plain_rejected[]
    }

    {
        using namespace polymorphic_classes;
        // tag::assign_nullptr[]
        Dog snoopy;
        virtual_ptr<Animal> p(snoopy);

        p = nullptr;

        BOOST_TEST(p.get() == nullptr);
        BOOST_TEST(p.vptr() == nullptr);
        // end::assign_nullptr[]
    }

    {
        using namespace polymorphic_classes;

        // tag::cast[]
        Dog snoopy;
        virtual_ptr<Animal> animal(snoopy);

        auto dog = animal.cast<Dog>();

        BOOST_TEST(dog.get() == &snoopy);
        BOOST_TEST(dog.vptr() == animal.vptr());
        // end::cast[]
    }

    {
        using namespace non_polymorphic_classes;
        capture_cout cout;

        // tag::final_virtual_ptr[]
        Dog snoopy;
        virtual_ptr<Animal> animal = final_virtual_ptr(snoopy);
        std::cout << poke(animal) << "\n"; // bark

        Cat felix;
        animal = final_virtual_ptr(felix);
        std::cout << poke(animal) << "\n"; // hiss
        // end::final_virtual_ptr[]

        BOOST_TEST(cout.str() == "bark\nhiss\n");
    }
}

BOOST_AUTO_TEST_CASE(shared_virtual_ptr_examples) {
    initialize();

    {
        using namespace non_polymorphic_classes;
        // tag::shared_ctor_default[]
        virtual_ptr<std::shared_ptr<Dog>> p;

        BOOST_TEST(p.get() == nullptr);
        BOOST_TEST(p.vptr() == nullptr);
        // end::shared_ctor_default[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::shared_ctor_nullptr[]
        virtual_ptr<std::shared_ptr<Dog>> p{nullptr};

        BOOST_TEST(p.get() == nullptr);
        BOOST_TEST(p.vptr() == nullptr);
        // end::shared_ctor_nullptr[]
    }

    {
        using namespace polymorphic_classes;
        // tag::shared_ctor_const_smart_ptr[]
        const std::shared_ptr<Dog> snoopy = std::make_shared<Dog>();
        virtual_ptr<std::shared_ptr<Animal>> p = snoopy;

        BOOST_TEST(p.get() == snoopy.get());
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::shared_ctor_const_smart_ptr[]
    }

    {
        using namespace polymorphic_classes;
        // tag::shared_ctor_smart_ptr[]
        std::shared_ptr<Dog> snoopy = std::make_shared<Dog>();
        virtual_ptr<std::shared_ptr<Animal>> p = snoopy;

        BOOST_TEST(p.get() == snoopy.get());
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::shared_ctor_smart_ptr[]
    }

    {
        using namespace polymorphic_classes;
        // tag::shared_ctor_move_smart_ptr[]
        std::shared_ptr<Dog> snoopy = std::make_shared<Dog>();
        Dog* moving = snoopy.get();

        virtual_ptr<std::shared_ptr<Animal>> p = std::move(snoopy);

        BOOST_TEST(p.get() == moving);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        BOOST_TEST(snoopy.get() == nullptr);
        // end::shared_ctor_move_smart_ptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::shared_ctor_const_vptr[]
        const virtual_ptr<std::shared_ptr<Dog>> snoopy =
            make_shared_virtual<Dog>();
        virtual_ptr<std::shared_ptr<Animal>> p = snoopy;

        BOOST_TEST(snoopy.get() != nullptr);
        BOOST_TEST(p.get() == snoopy.get());
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::shared_ctor_const_vptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::shared_ctor_move_vptr[]
        virtual_ptr<std::shared_ptr<Dog>> snoopy = make_shared_virtual<Dog>();
        Dog* dog = snoopy.get();

        virtual_ptr<std::shared_ptr<Animal>> p = std::move(snoopy);

        BOOST_TEST(p.get() == dog);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        BOOST_TEST(snoopy.get() == nullptr);
        // end::shared_ctor_move_vptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::shared_assign_nullptr[]
        virtual_ptr<std::shared_ptr<Dog>> p = make_shared_virtual<Dog>();

        p = nullptr;

        BOOST_TEST(p.get() == nullptr);
        BOOST_TEST(p.vptr() == nullptr);
        BOOST_TEST((p == virtual_ptr<std::shared_ptr<Dog>>()));
        // end::shared_assign_nullptr[]
    }

    {
        using namespace polymorphic_classes;
        // tag::shared_assign_smart_ptr[]
        std::shared_ptr<Dog> snoopy = std::make_shared<Dog>();
        virtual_ptr<std::shared_ptr<Animal>> p;

        p = snoopy;

        BOOST_TEST(p.get() == snoopy.get());
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        // end::shared_assign_smart_ptr[]
    }

    {
        using namespace polymorphic_classes;
        // tag::shared_assign_move_smart_ptr[]
        std::shared_ptr<Dog> snoopy = std::make_shared<Dog>();
        Dog* moving = snoopy.get();
        virtual_ptr<std::shared_ptr<Animal>> p;

        p = std::move(snoopy);

        BOOST_TEST(p.get() == moving);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        BOOST_TEST(snoopy.get() == nullptr);
        // end::shared_assign_move_smart_ptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::shared_assign_vptr[]
        virtual_ptr<std::shared_ptr<Dog>> snoopy = make_shared_virtual<Dog>();
        virtual_ptr<std::shared_ptr<Dog>> p;

        p = snoopy;

        BOOST_TEST(p.get() != nullptr);
        BOOST_TEST(p.get() == snoopy.get());
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        BOOST_TEST(snoopy.vptr() == default_registry::static_vptr<Dog>);
        // end::shared_assign_vptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::shared_assign_const_vptr[]
        const virtual_ptr<std::shared_ptr<Dog>> snoopy =
            make_shared_virtual<Dog>();
        virtual_ptr<std::shared_ptr<Dog>> p;

        p = snoopy;

        BOOST_TEST(p.get() != nullptr);
        BOOST_TEST(p.get() == snoopy.get());
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        BOOST_TEST(snoopy.vptr() == default_registry::static_vptr<Dog>);
        // end::shared_assign_const_vptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::shared_assign_move_vptr[]
        virtual_ptr<std::shared_ptr<Dog>> snoopy = make_shared_virtual<Dog>();
        Dog* moving = snoopy.get();
        virtual_ptr<std::shared_ptr<Dog>> p;

        p = std::move(snoopy);

        BOOST_TEST(p.get() == moving);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        BOOST_TEST(snoopy.get() == nullptr);
        BOOST_TEST(snoopy.vptr() == nullptr);
        // end::shared_assign_move_vptr[]
    }
}

BOOST_AUTO_TEST_CASE(unique_virtual_ptr_examples) {
    initialize();

    {
        using namespace polymorphic_classes;
        // tag::unique_copy_rejected[]
        static_assert(
            std::is_constructible_v<
                unique_virtual_ptr<Animal>, const std::unique_ptr<Dog>&> ==
            false);
        // end::unique_copy_rejected[]
    }

    {
        using namespace polymorphic_classes;
        // tag::unique_ctor_move_smart_ptr[]
        std::unique_ptr<Dog> snoopy = std::make_unique<Dog>();
        Dog* moving = snoopy.get();

        unique_virtual_ptr<Animal> p = std::move(snoopy);

        BOOST_TEST(p.get() == moving);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        BOOST_TEST(snoopy.get() == nullptr);
        // end::unique_ctor_move_smart_ptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::unique_ctor_move_vptr[]
        unique_virtual_ptr<Dog> snoopy = make_unique_virtual<Dog>();
        Dog* moving = snoopy.get();

        unique_virtual_ptr<Animal> p = std::move(snoopy);

        BOOST_TEST(p.get() == moving);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        BOOST_TEST(snoopy.get() == nullptr);
        // end::unique_ctor_move_vptr[]
    }

    {
        using namespace polymorphic_classes;
        // tag::unique_assign_move_smart_ptr[]
        std::unique_ptr<Dog> snoopy = std::make_unique<Dog>();
        Dog* moving = snoopy.get();
        unique_virtual_ptr<Animal> p;

        p = std::move(snoopy);

        BOOST_TEST(p.get() == moving);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        BOOST_TEST(snoopy.get() == nullptr);
        // end::unique_assign_move_smart_ptr[]
    }

    {
        using namespace non_polymorphic_classes;
        // tag::unique_assign_move_vptr[]
        unique_virtual_ptr<Dog> snoopy = make_unique_virtual<Dog>();
        Dog* moving = snoopy.get();
        unique_virtual_ptr<Dog> p;

        p = std::move(snoopy);

        BOOST_TEST(p.get() == moving);
        BOOST_TEST(p.vptr() == default_registry::static_vptr<Dog>);
        BOOST_TEST(snoopy.get() == nullptr);
        BOOST_TEST(snoopy.vptr() == nullptr);
        // end::unique_assign_move_vptr[]
    }
}
