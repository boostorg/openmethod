// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/any.hpp>
#include <string>
#include <utility>

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/boost_any.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>

#include "test_classes.hpp"

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

#define MAKE_CLASSES()                                                         \
    struct Dog {                                                               \
        std::string name;                                                      \
    };

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as const virtual_boost_any& (const ref)

// A virtual_any hands out the v-table pointer it cached, through its
// virtual_traits, for all three reference categories.
static_assert(detail::has_vptr<
              virtual_traits<const virtual_boost_any&, default_registry>,
              const virtual_boost_any&>);
static_assert(detail::has_vptr<
              virtual_traits<virtual_boost_any&, default_registry>,
              const virtual_boost_any&>);
static_assert(detail::has_vptr<
              virtual_traits<virtual_boost_any&&, default_registry>,
              const virtual_boost_any&>);

MAKE_CLASSES();

BOOST_OPENMETHOD(name, (const virtual_boost_any&), std::string);

// The overriders can use the macro: the value constructor of virtual_any
// makes the overrider's parameter convertible to the method's, so the
// method is located.

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(name, (const std::string& name), std::string) {
    return name;
}

// A catch-all overrider may keep the wrapper.
BOOST_OPENMETHOD_OVERRIDE(name, (const virtual_boost_any& va), std::string) {
    return !va.get().empty() ? "something" : "nothing";
}

BOOST_AUTO_TEST_CASE(virtual_any_by_const_ref) {
    initialize(trace());

    // from an `any`: the v-table pointer is looked up from the dynamic
    // type of the contained value
    const boost::any spot_any(Dog{"Spot"});
    virtual_boost_any spot = spot_any;
    BOOST_TEST(spot.vptr() == default_registry::static_vptr<Dog>);
    BOOST_TEST(name(spot) == "Spot the dog");

    // from a value: the v-table pointer is set statically
    virtual_boost_any rex = Dog{"Rex"};
    BOOST_TEST(rex.vptr() == default_registry::static_vptr<Dog>);
    BOOST_TEST(name(rex) == "Rex the dog");

    virtual_boost_any felix = std::string("Felix the cat");
    BOOST_TEST(felix.vptr() == default_registry::static_vptr<std::string>);
    BOOST_TEST(name(felix) == "Felix the cat");

    // a value converts to a (temporary) virtual_any at the call site
    BOOST_TEST(name(Dog{"Fido"}) == "Fido the dog");

    // `int` is registered automatically - the value conversion at the call
    // site stores it - but has no specific overrider: the catch-all,
    // registered for the `boost::any` root, applies
    BOOST_TEST(name(42) == "something");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as virtual_boost_any& (mutable ref)

MAKE_CLASSES();

BOOST_OPENMETHOD(bump, (virtual_boost_any&), std::string);

// BOOST_OPENMETHOD_OVERRIDE cannot express this: a temporary virtual_any
// binds to `const virtual_boost_any&` and to `virtual_boost_any&&`, but
// nothing binds to a mutable lvalue reference. Register directly via
// method<...>::override<Fn> instead - the primitive the macro itself
// expands to.

using bump_method =
    BOOST_OPENMETHOD_TYPE(bump, (virtual_boost_any&), std::string);

auto bump_dog(Dog& dog) -> std::string {
    dog.name += " Jr.";
    return dog.name + " the dog";
}

auto bump_int(int& value) -> std::string {
    ++value;
    return "bumped";
}

BOOST_OPENMETHOD_REGISTER(bump_method::override<bump_dog>);
BOOST_OPENMETHOD_REGISTER(bump_method::override<bump_int>);

BOOST_AUTO_TEST_CASE(virtual_any_by_mutable_ref) {
    initialize(trace());

    virtual_boost_any spot = Dog{"Spot"};
    BOOST_TEST(bump(spot) == "Spot Jr. the dog");
    // the mutation is visible through the virtual_any
    BOOST_TEST(boost::any_cast<const Dog&>(spot.get()).name == "Spot Jr.");

    virtual_boost_any answer = 41;
    BOOST_TEST(bump(answer) == "bumped");
    BOOST_TEST(boost::any_cast<const int&>(answer.get()) == 42);
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as virtual_boost_any&& (xvalue ref)

MAKE_CLASSES();

BOOST_OPENMETHOD(steal, (virtual_boost_any&&), std::string);

BOOST_OPENMETHOD_OVERRIDE(steal, (Dog && dog), std::string) {
    Dog stolen(std::move(dog));
    return stolen.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(steal, (std::string && name), std::string) {
    std::string stolen(std::move(name));
    return stolen;
}

BOOST_AUTO_TEST_CASE(virtual_any_by_xvalue_ref) {
    initialize(trace());

    virtual_boost_any spot = Dog{"Spot"};
    BOOST_TEST(steal(std::move(spot)) == "Spot the dog");
    // the overrider moved the name out; the virtual_any still owns the Dog
    BOOST_TEST(!spot.get().empty());
    BOOST_TEST(boost::any_cast<const Dog&>(spot.get()).name == "");

    BOOST_TEST(
        steal(virtual_boost_any(std::string("Felix the cat"))) ==
        "Felix the cat");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// value semantics

MAKE_CLASSES();

BOOST_OPENMETHOD(name, (const virtual_boost_any&), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

BOOST_AUTO_TEST_CASE(virtual_any_value_semantics) {
    initialize(trace());

    virtual_boost_any empty;
    BOOST_TEST(empty.get().empty());
    BOOST_TEST(empty.vptr() == nullptr);

    virtual_boost_any rex = Dog{"Rex"};

    // copy: independent payloads, same v-table pointer
    auto copy = rex;
    BOOST_TEST(copy.vptr() == rex.vptr());
    BOOST_TEST(name(copy) == "Rex the dog");
    BOOST_TEST(name(rex) == "Rex the dog"); // original unaffected

    // move: the source's v-table pointer is nulled
    auto moved = std::move(copy);
    BOOST_TEST(moved.vptr() == default_registry::static_vptr<Dog>);
    BOOST_TEST(copy.vptr() == nullptr);
    BOOST_TEST(name(moved) == "Rex the dog");

    // assignment from an `any` re-derives the v-table pointer;
    // `std::string` is registered by the `emplace` below
    boost::any felix_any(std::string{"Felix"});
    moved = felix_any;
    BOOST_TEST(moved.vptr() == default_registry::static_vptr<std::string>);

    // assignment from a value sets it statically
    moved = Dog{"Snoopy"};
    BOOST_TEST(moved.vptr() == default_registry::static_vptr<Dog>);
    BOOST_TEST(name(moved) == "Snoopy the dog");

    // emplace constructs in place and sets it statically
    moved.emplace<std::string>("Sylvester");
    BOOST_TEST(moved.vptr() == default_registry::static_vptr<std::string>);
    BOOST_TEST(boost::any_cast<const std::string&>(moved.get()) == "Sylvester");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// indirect vptrs

struct Dog {
    std::string name;
};

// `Dog` is registered in `indirect_registry` - the method's registry - by
// the overrider and by the value constructor

using name_method = method<
    struct name_id,
    std::string(const virtual_any<boost::any, indirect_registry>&),
    indirect_registry>;

auto name_dog(const Dog& dog) -> std::string {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_REGISTER(name_method::override<name_dog>);

BOOST_AUTO_TEST_CASE(virtual_any_indirect_vptr) {
    initialize<indirect_registry>();

    boost::any spot_any(Dog{"Spot"});
    virtual_any<boost::any, indirect_registry> spot = spot_any;
    BOOST_TEST(spot.vptr() == indirect_registry::static_vptr<Dog>);
    BOOST_TEST(name_method::fn(spot) == "Spot the dog");

    virtual_any<boost::any, indirect_registry> rex = Dog{"Rex"};
    BOOST_TEST(name_method::fn(rex) == "Rex the dog");
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

using throw_any = virtual_any<boost::any, throw_registry>;

using name_method =
    method<struct name_id, std::string(const throw_any&), throw_registry>;

auto name_dog(const Dog& dog) -> std::string {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_REGISTER(name_method::override<name_dog>);

BOOST_AUTO_TEST_CASE(virtual_any_unregistered_type) {
    initialize<throw_registry>();

    // `double` is named nowhere: it is not registered, and constructing or
    // assigning a virtual_any from an `any` containing one is a
    // missing_class error at the construction/assignment site
    boost::any pi(3.14);
    BOOST_CHECK_THROW(throw_any{pi}, missing_class);

    throw_any va;
    BOOST_CHECK_THROW(va = pi, missing_class);
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// mixing with ordinary virtual parameters

MAKE_CLASSES();

struct Animal {
    virtual ~Animal() {
    }
};

struct Cat : Animal {};

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Cat);

BOOST_OPENMETHOD(
    meet, (const virtual_boost_any&, virtual_ptr<const Animal>), std::string);

BOOST_OPENMETHOD_OVERRIDE(
    meet, (const Dog& dog, virtual_ptr<const Cat>), std::string) {
    return dog.name + " meets a cat";
}

BOOST_OPENMETHOD_OVERRIDE(
    meet, (const virtual_boost_any&, virtual_ptr<const Animal>), std::string) {
    return "someone meets an animal";
}

BOOST_AUTO_TEST_CASE(virtual_any_mixed_with_virtual_ptr) {
    initialize(trace());

    virtual_boost_any spot = Dog{"Spot"};
    virtual_boost_any pi = 3.14f;
    Cat felix;

    BOOST_TEST(meet(spot, felix) == "Spot meets a cat");
    BOOST_TEST(meet(pi, felix) == "someone meets an animal");
}
} // namespace BOOST_OPENMETHOD_GENSYM

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
