// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// static_rtti and named_static_rtti have no dynamic_type. A bad_call still
// reports the class of each argument: the one whose v-table the virtual_ptr
// points to.

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/static_rtti.hpp>
#include <boost/openmethod/policies/named_static_rtti.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>
#include <boost/openmethod/inplace_vptr.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include <boost/mp11/list.hpp>

#include <sstream>
#include <string_view>
#include <type_traits>

using namespace boost::openmethod;

struct static_registry :
    registry<policies::static_rtti, policies::throw_error_handler> {};

struct named_registry :
    registry<policies::named_static_rtti, policies::throw_error_handler> {};

struct Animal {};

struct Dog : Animal {};

struct Cat : Animal {};

struct BOOST_OPENMETHOD_ID(speak);
struct BOOST_OPENMETHOD_ID(meet);

template<class Registry>
void speak_cat(virtual_ptr<Cat, Registry>) {
}

template<class Registry>
void meet_dog_animal(
    virtual_ptr<Dog, Registry>, virtual_ptr<Animal, Registry>) {
}

template<class Registry>
void meet_animal_dog(
    virtual_ptr<Animal, Registry>, virtual_ptr<Dog, Registry>) {
}

// Checks that `call` reports `Error`, and that every argument is reported as a
// Dog - the class of the object, not Animal, the class of the parameter.
template<class Registry, class Error, class Call>
void check_error(Call call, std::size_t arity) {
    using rtti = typename Registry::rtti;

    try {
        call();
        BOOST_FAIL("should have thrown");
    } catch (const Error& error) {
        BOOST_TEST(error.arity == arity);

        for (std::size_t i = 0; i < arity; ++i) {
            BOOST_TEST(
                (rtti::type_index(error.types[i]) ==
                    rtti::type_index(rtti::template static_type<Dog>())));

            if constexpr (std::is_same_v<Registry, named_registry>) {
                std::ostringstream os;
                rtti::type_name(error.types[i], os);
                BOOST_TEST(os.str() == "Dog");
            }
        }
    } catch (...) {
        BOOST_FAIL("wrong exception");
    }
}

using registries = boost::mp11::mp_list<static_registry, named_registry>;

BOOST_AUTO_TEST_CASE_TEMPLATE(bad_call_types, Registry, registries) {
    BOOST_OPENMETHOD_REGISTER(use_classes<Animal, Dog, Cat, Registry>);

    using speak = method<
        BOOST_OPENMETHOD_ID(speak), void(virtual_ptr<Animal, Registry>),
        Registry>;
    BOOST_OPENMETHOD_REGISTER(
        typename speak::template override<speak_cat<Registry>>);

    using meet = method<
        BOOST_OPENMETHOD_ID(meet),
        void(virtual_ptr<Animal, Registry>, virtual_ptr<Animal, Registry>),
        Registry>;
    BOOST_OPENMETHOD_REGISTER(
        typename meet::template override<
            meet_dog_animal<Registry>, meet_animal_dog<Registry>>);

    initialize<Registry>();
    Dog dog;

    check_error<Registry, no_overrider>(
        [&] { speak::fn(final_virtual_ptr<Registry>(dog)); }, 1);
    check_error<Registry, no_overrider>(
        [&] { speak::fn(make_unique_virtual<Dog, Registry>()); }, 1);
    check_error<Registry, ambiguous_call>(
        [&] {
            meet::fn(
                final_virtual_ptr<Registry>(dog),
                final_virtual_ptr<Registry>(dog));
        },
        2);
}

// A class that carries its own v-table pointer needs no dynamic_type: the
// class is read from the pointer. The classes must not be polymorphic in the
// C++ sense: inplace_vptr_base's constructor casts `this` to the derived
// class, which UBSan rejects while the C++ v-table pointer is not set yet.
struct inplace_registry :
    registry<policies::static_rtti, policies::throw_error_handler> {};

struct Shape : inplace_vptr_base<Shape, inplace_registry> {};

struct Square : Shape, inplace_vptr_derived<Square, Shape> {};

BOOST_OPENMETHOD_CLASSES(Shape, Square, inplace_registry);

BOOST_OPENMETHOD(draw, (virtual_<Shape&>), void, inplace_registry);

BOOST_AUTO_TEST_CASE(bad_call_types_inplace_vptr) {
    using rtti = inplace_registry::rtti;

    initialize<inplace_registry>();
    Square square;

    try {
        draw(square);
        BOOST_FAIL("should have thrown");
    } catch (const no_overrider& error) {
        BOOST_TEST(error.types[0] == rtti::static_type<Square>());
    } catch (...) {
        BOOST_FAIL("wrong exception");
    }
}
