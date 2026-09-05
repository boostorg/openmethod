// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <boost/any.hpp>

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/boost_any.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>

#include "test_classes.hpp"

#define BOOST_TEST_MODULE dispatch_boost_any
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

#define MAKE_CLASSES()                                                         \
    struct Dog {                                                               \
        std::string name;                                                      \
    };

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as const boost::any& (const ref)

static_assert(detail::has_vptr<
              virtual_traits<const boost::any&, default_registry>,
              const boost::any&>);

MAKE_CLASSES();

BOOST_OPENMETHOD(name, (virtual_<const boost::any&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(name, (const std::string& name), std::string) {
    return name;
}

BOOST_OPENMETHOD_OVERRIDE(name, (const int& value), std::string) {
    std::ostringstream os;
    os << value << " the integer";
    return os.str();
}

// A catch-all overrider may take the `any` itself; the argument is passed
// through unchanged, instead of going through boost::any_cast, which
// would throw unless the `any` contains an `any`.

BOOST_OPENMETHOD_OVERRIDE(name, (const boost::any& arg), std::string) {
    return !arg.empty() ? "something" : "nothing";
}

// `double` is registered because `weigh`'s overrider names it; it has no
// `name` overrider, so the catch-all applies to it.

BOOST_OPENMETHOD(weigh, (virtual_<const boost::any&>), double);

BOOST_OPENMETHOD_OVERRIDE(weigh, (const double& value), double) {
    return value;
}

BOOST_AUTO_TEST_CASE(boost_any_by_const_ref) {
    initialize(trace());

    const boost::any spot(Dog{"Spot"});
    const boost::any felix(std::string{"Felix the cat"});
    const boost::any answer(42);

    BOOST_TEST(name(spot) == "Spot the dog");
    BOOST_TEST(name(felix) == "Felix the cat");
    BOOST_TEST(name(answer) == "42 the integer");

    // `double` is registered, but has no specific overrider: the catch-all,
    // registered for the `boost::any` root, applies
    BOOST_TEST(name(boost::any(1.5)) == "something");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as boost::any& (mutable ref)

static_assert(
    detail::has_vptr<
        virtual_traits<boost::any&, default_registry>, const boost::any&>);

MAKE_CLASSES();

BOOST_OPENMETHOD(bump, (virtual_<boost::any&>), std::string);

// BOOST_OPENMETHOD_OVERRIDE cannot express this. It locates the method by
// checking that the overrider's parameter types can be passed to the method's
// forwarder (see enable_forwarder and the guide function in macros.hpp), and
// `Dog&` does not convert to `boost::any&`. A temporary `boost::any` binds to
// `const boost::any&` and to `boost::any&&`, which is why the other two
// reference categories can use the macro; nothing binds to a mutable lvalue
// reference. Register directly via method<...>::override<Fn> instead - the
// primitive the macro itself expands to.

using bump_method =
    BOOST_OPENMETHOD_TYPE(bump, (virtual_<boost::any&>), std::string);

auto bump_dog(Dog& dog) -> std::string {
    dog.name += " Jr.";
    return dog.name + " the dog";
}

auto bump_string(std::string& name) -> std::string {
    name += "!";
    return name;
}

auto bump_int(int& value) -> std::string {
    ++value;
    std::ostringstream os;
    os << value << " the integer";
    return os.str();
}

BOOST_OPENMETHOD_REGISTER(bump_method::override<bump_dog>);
BOOST_OPENMETHOD_REGISTER(bump_method::override<bump_string>);
BOOST_OPENMETHOD_REGISTER(bump_method::override<bump_int>);

BOOST_AUTO_TEST_CASE(boost_any_by_mutable_ref) {
    initialize(trace());

    boost::any spot(Dog{"Spot"});
    boost::any felix(std::string{"Felix the cat"});
    boost::any answer(41);

    BOOST_TEST(bump(spot) == "Spot Jr. the dog");
    BOOST_TEST(boost::any_cast<const Dog&>(spot).name == "Spot Jr.");

    BOOST_TEST(bump(felix) == "Felix the cat!");
    BOOST_TEST(boost::any_cast<const std::string&>(felix) == "Felix the cat!");

    BOOST_TEST(bump(answer) == "42 the integer");
    BOOST_TEST(boost::any_cast<const int&>(answer) == 42);
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as boost::any&& (xvalue ref)

static_assert(
    detail::has_vptr<
        virtual_traits<boost::any&&, default_registry>, const boost::any&>);

MAKE_CLASSES();

BOOST_OPENMETHOD(steal, (virtual_<boost::any&&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(steal, (Dog && dog), std::string) {
    Dog stolen(std::move(dog));
    return stolen.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(steal, (std::string && name), std::string) {
    std::string stolen(std::move(name));
    return stolen;
}

BOOST_OPENMETHOD_OVERRIDE(steal, (int&& value), std::string) {
    std::ostringstream os;
    os << value << " the integer";
    return os.str();
}

BOOST_AUTO_TEST_CASE(boost_any_by_xvalue_ref) {
    initialize(trace());

    boost::any spot(Dog{"Spot"});
    BOOST_TEST(steal(std::move(spot)) == "Spot the dog");
    // the overrider moved the name out; the `any` still owns the Dog
    BOOST_TEST(!spot.empty());
    BOOST_TEST(boost::any_cast<const Dog&>(spot).name == "");

    boost::any felix(std::string{"Felix the cat"});
    BOOST_TEST(steal(std::move(felix)) == "Felix the cat");
    BOOST_TEST(!felix.empty());
    BOOST_TEST(boost::any_cast<const std::string&>(felix) == "");

    // moving an int copies it
    boost::any answer(42);
    BOOST_TEST(steal(std::move(answer)) == "42 the integer");
    BOOST_TEST(boost::any_cast<const int&>(answer) == 42);
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// catch-all overriders

// `float` has no overrider of the methods under test; it is registered
// because `weigh`'s overrider names it. The catch-alls apply to it.

#define MAKE_CATCH_ALL_CLASSES()                                               \
    struct Dog {                                                               \
        std::string name;                                                      \
    };                                                                         \
                                                                               \
    BOOST_OPENMETHOD(weigh, (virtual_<const boost::any&>), float);             \
                                                                               \
    BOOST_OPENMETHOD_OVERRIDE(weigh, (const float& value), float) {            \
        return value;                                                          \
    }

MAKE_CATCH_ALL_CLASSES();

// An overrider may take the `any` itself. Since every registered type
// derives from it, such an overrider is a catch-all, applying to any
// contained type that has no more specific overrider.

BOOST_OPENMETHOD(name, (virtual_<const boost::any&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(name, (const boost::any&), std::string) {
    return "something else";
}

BOOST_OPENMETHOD(bump, (virtual_<boost::any&>), std::string);

using bump_method =
    BOOST_OPENMETHOD_TYPE(bump, (virtual_<boost::any&>), std::string);

auto bump_dog(Dog& dog) -> std::string {
    dog.name += " Jr.";
    return dog.name + " the dog";
}

auto bump_any(boost::any&) -> std::string {
    return "something else";
}

BOOST_OPENMETHOD_REGISTER(bump_method::override<bump_dog>);
BOOST_OPENMETHOD_REGISTER(bump_method::override<bump_any>);

BOOST_OPENMETHOD(steal, (virtual_<boost::any&&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(steal, (Dog && dog), std::string) {
    Dog stolen(std::move(dog));
    return stolen.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(steal, (boost::any&&), std::string) {
    return "something else";
}

BOOST_AUTO_TEST_CASE(boost_any_catch_all) {
    initialize(trace());

    boost::any spot(Dog{"Spot"});
    boost::any pi(3.14f);

    BOOST_TEST(weigh(pi) == 3.14f);

    BOOST_TEST(name(spot) == "Spot the dog");
    BOOST_TEST(name(pi) == "something else");

    BOOST_TEST(bump(spot) == "Spot Jr. the dog");
    BOOST_TEST(bump(pi) == "something else");

    BOOST_TEST(steal(boost::any(Dog{"Fido"})) == "Fido the dog");
    BOOST_TEST(steal(std::move(pi)) == "something else");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// `any` and ordinary virtual parameters mixed in one method

MAKE_CATCH_ALL_CLASSES();

struct Animal {
    virtual ~Animal() {
    }
};

struct Cat : Animal {};

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Cat);

BOOST_OPENMETHOD(
    meet, (virtual_<const boost::any&>, virtual_ptr<const Animal>),
    std::string);

BOOST_OPENMETHOD_OVERRIDE(
    meet, (const Dog& dog, virtual_ptr<const Cat>), std::string) {
    return dog.name + " meets a cat";
}

BOOST_OPENMETHOD_OVERRIDE(
    meet, (const boost::any&, virtual_ptr<const Animal>), std::string) {
    return "someone meets an animal";
}

BOOST_AUTO_TEST_CASE(boost_any_mixed_with_virtual_ptr) {
    initialize(trace());

    boost::any spot(Dog{"Spot"});
    boost::any pi(3.14f);
    Cat felix;

    BOOST_TEST(meet(spot, felix) == "Spot meets a cat");
    BOOST_TEST(meet(pi, felix) == "someone meets an animal");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// a type that is never named statically anywhere is not registered

struct throw_registry :
    default_registry::with<
        policies::runtime_checks, policies::throw_error_handler> {};

struct Dog {
    std::string name;
};

using name_method = method<
    struct name_id, std::string(virtual_<const boost::any&>), throw_registry>;

auto name_dog(const Dog& dog) -> std::string {
    return dog.name + " the dog";
}

auto name_any(const boost::any&) -> std::string {
    return "something else";
}

BOOST_OPENMETHOD_REGISTER(name_method::override<name_dog>);
BOOST_OPENMETHOD_REGISTER(name_method::override<name_any>);

BOOST_AUTO_TEST_CASE(boost_any_unregistered_type) {
    initialize<throw_registry>();

    // `double` is named nowhere: it is not registered, and even the
    // catch-all does not apply; the v-table lookup is a missing_class error
    boost::any pi(3.14);
    BOOST_CHECK_THROW(name_method::fn(pi), missing_class);
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// a class may belong to a hierarchy and be dispatched via an `any` as well

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {
    explicit Dog(std::string name) : name(std::move(name)) {
    }

    std::string name;
};

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog);

BOOST_OPENMETHOD(poke, (virtual_ptr<const Animal>), std::string);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<const Dog> dog), std::string) {
    return dog->name + " barks";
}

BOOST_OPENMETHOD(name, (virtual_<const boost::any&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

BOOST_AUTO_TEST_CASE(boost_any_class_in_hierarchy) {
    initialize(trace());

    // `Dog` has two bases in the lattice: `Animal`, registered explicitly,
    // and `boost::any`, added because `name`'s overrider names `Dog`
    Dog spot("Spot");
    BOOST_TEST(poke(spot) == "Spot barks");

    boost::any any_spot(Dog{"Spot"});
    BOOST_TEST(name(any_spot) == "Spot the dog");
}
} // namespace BOOST_OPENMETHOD_GENSYM

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
