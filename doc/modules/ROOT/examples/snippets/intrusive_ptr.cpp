// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/boost_intrusive_ptr.hpp>

#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "capture.hpp"

using namespace boost::openmethod;

// tag::classes[]
struct Animal : boost::intrusive_ref_counter<Animal> {
    virtual ~Animal() = default;
};
struct Dog : Animal {};
struct Cat : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);
// end::classes[]

namespace by_value {

// tag::by_value[]
BOOST_OPENMETHOD(poke, (virtual_<boost::intrusive_ptr<Animal>>), std::string);

BOOST_OPENMETHOD_OVERRIDE(
    poke, (boost::intrusive_ptr<Dog> animal), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(
    poke, (boost::intrusive_ptr<Cat> animal), std::string) {
    return "hiss";
}
// end::by_value[]

} // namespace by_value

namespace by_reference {

// tag::by_reference[]
BOOST_OPENMETHOD(
    poke, (virtual_<const boost::intrusive_ptr<Animal>&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(
    poke, (const boost::intrusive_ptr<Dog>& animal), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(
    poke, (const boost::intrusive_ptr<Cat>& animal), std::string) {
    return "hiss";
}
// end::by_reference[]

} // namespace by_reference

namespace vptr {

BOOST_OPENMETHOD(poke, (boost_intrusive_virtual_ptr<Animal>), std::string);

BOOST_OPENMETHOD_OVERRIDE(
    poke, (boost_intrusive_virtual_ptr<Dog> animal), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(
    poke, (boost_intrusive_virtual_ptr<Cat> animal), std::string) {
    return "hiss";
}

} // namespace vptr

BOOST_AUTO_TEST_CASE(intrusive_ptr_examples) {
    initialize();

    {
        using namespace vptr;
        capture_cout cout;

        // tag::make_boost_intrusive_virtual[]
        boost_intrusive_virtual_ptr<Animal> animal =
            make_boost_intrusive_virtual<Dog>();

        std::cout << poke(animal) << "\n"; // bark
        // end::make_boost_intrusive_virtual[]

        BOOST_TEST(cout.str() == "bark\n");
    }

    {
        // tag::boost_intrusive_virtual_ptr_alias[]
        boost_intrusive_virtual_ptr<Animal> animal =
            make_boost_intrusive_virtual<Dog>();
        boost::intrusive_ptr<Animal> owner = animal.pointer();

        BOOST_TEST(owner->use_count() == 2);
        // end::boost_intrusive_virtual_ptr_alias[]
    }

    {
        using namespace by_value;
        capture_cout cout;

        // tag::by_value_call[]
        std::cout << poke(boost::intrusive_ptr<Animal>(new Dog))
                  << "\n"; // bark
        std::cout << poke(boost::intrusive_ptr<Animal>(new Cat))
                  << "\n"; // hiss
        // end::by_value_call[]

        BOOST_TEST(cout.str() == "bark\nhiss\n");
    }

    {
        using namespace by_reference;
        capture_cout cout;

        // tag::by_reference_call[]
        const boost::intrusive_ptr<Animal> snoopy(new Dog);

        std::cout << poke(snoopy) << "\n"; // bark

        BOOST_TEST(snoopy->use_count() == 1);
        // end::by_reference_call[]

        BOOST_TEST(cout.str() == "bark\n");
    }
}
