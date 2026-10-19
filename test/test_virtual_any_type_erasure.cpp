// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <utility>

#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/builtin.hpp>
#include <boost/type_erasure/is_empty.hpp>
#include <boost/mpl/vector.hpp>

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/boost_type_erasure.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

namespace te = boost::type_erasure;
using namespace boost::openmethod;

// `relaxed` is needed for virtual_any's default constructor and
// assignment, `copy_constructible<>` for its copies. `relaxed` also
// implies typeid_<>, on which the v-table lookup relies.
using Concept = boost::mpl::vector<te::copy_constructible<>, te::relaxed>;
using erased = te::any<Concept>;
using virtual_erased = virtual_any<erased>;

#define MAKE_CLASSES()                                                         \
    struct Dog {                                                               \
        std::string name;                                                      \
    };

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as const virtual_any<erased>& (const ref)

MAKE_CLASSES();

BOOST_OPENMETHOD(name, (const virtual_erased&), std::string);

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
BOOST_OPENMETHOD_OVERRIDE(name, (const virtual_erased& va), std::string) {
    return !te::is_empty(va.get()) ? "something" : "nothing";
}

BOOST_AUTO_TEST_CASE(virtual_any_by_const_ref) {
    initialize(trace());

    // from an `any`: the v-table pointer is looked up from the type bound
    // to the `any`
    const erased spot_any(Dog{"Spot"});
    virtual_erased spot = spot_any;
    BOOST_TEST(spot.vptr() == default_registry::static_vptr<Dog>);
    BOOST_TEST(name(spot) == "Spot the dog");

    // from a value: the v-table pointer is set statically
    virtual_erased rex = Dog{"Rex"};
    BOOST_TEST(rex.vptr() == default_registry::static_vptr<Dog>);
    BOOST_TEST(name(rex) == "Rex the dog");

    virtual_erased felix = std::string("Felix the cat");
    BOOST_TEST(felix.vptr() == default_registry::static_vptr<std::string>);
    BOOST_TEST(name(felix) == "Felix the cat");

    // a value converts to a (temporary) virtual_any at the call site
    BOOST_TEST(name(Dog{"Fido"}) == "Fido the dog");

    // `int` is registered automatically - the value conversion at the call
    // site stores it - but has no specific overrider: the catch-all,
    // registered for the owning any, applies
    BOOST_TEST(name(42) == "something");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as virtual_any<erased>& (mutable ref)

MAKE_CLASSES();

BOOST_OPENMETHOD(bump, (virtual_erased&), std::string);

// BOOST_OPENMETHOD_OVERRIDE cannot express this: a temporary virtual_any
// binds to `const virtual_erased&` and to `virtual_erased&&`, but nothing
// binds to a mutable lvalue reference. Register directly via
// method<...>::override<Fn> instead - the primitive the macro itself
// expands to.

using bump_method = BOOST_OPENMETHOD_TYPE(bump, (virtual_erased&), std::string);

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

    virtual_erased spot = Dog{"Spot"};
    BOOST_TEST(bump(spot) == "Spot Jr. the dog");
    // the mutation is visible through the virtual_any
    BOOST_TEST(te::any_cast<const Dog&>(spot.get()).name == "Spot Jr.");

    virtual_erased answer = 41;
    BOOST_TEST(bump(answer) == "bumped");
    BOOST_TEST(te::any_cast<const int&>(answer.get()) == 42);
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as virtual_any<erased>&& (xvalue ref)

MAKE_CLASSES();

BOOST_OPENMETHOD(steal, (virtual_erased&&), std::string);

// boost::type_erasure::any_cast has no rvalue overload; the trait moves
// the result of a mutable-reference cast, because the `any` owns its
// value.
BOOST_OPENMETHOD_OVERRIDE(steal, (Dog&& dog), std::string) {
    Dog stolen(std::move(dog));

    return stolen.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(steal, (std::string&& name), std::string) {
    std::string stolen(std::move(name));

    return stolen;
}

BOOST_AUTO_TEST_CASE(virtual_any_by_xvalue_ref) {
    initialize(trace());

    virtual_erased spot = Dog{"Spot"};
    BOOST_TEST(steal(std::move(spot)) == "Spot the dog");
    // the overrider moved the name out; the virtual_any still owns the Dog
    // coverity[use_after_move]
    BOOST_TEST(!te::is_empty(spot.get()));
    BOOST_TEST(te::any_cast<const Dog&>(spot.get()).name == "");

    BOOST_TEST(
        steal(virtual_erased(std::string("Felix the cat"))) == "Felix the cat");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// value semantics

MAKE_CLASSES();

BOOST_OPENMETHOD(name, (const virtual_erased&), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

BOOST_AUTO_TEST_CASE(virtual_any_value_semantics) {
    initialize(trace());

    virtual_erased empty;
    BOOST_TEST(te::is_empty(empty.get()));
    BOOST_TEST(empty.vptr() == nullptr);

    virtual_erased rex = Dog{"Rex"};

    // copy: independent payloads, same v-table pointer
    auto copy = rex;
    BOOST_TEST(copy.vptr() == rex.vptr());
    BOOST_TEST(name(copy) == "Rex the dog");
    BOOST_TEST(name(rex) == "Rex the dog"); // original unaffected

    // move: the source's v-table pointer is nulled
    auto moved = std::move(copy);
    BOOST_TEST(moved.vptr() == default_registry::static_vptr<Dog>);
    // coverity[use_after_move]
    BOOST_TEST(copy.vptr() == nullptr);
    BOOST_TEST(name(moved) == "Rex the dog");

    // assignment from an `any` re-derives the v-table pointer;
    // `std::string` is registered by the `emplace` below
    erased felix_any(std::string{"Felix"});
    moved = felix_any;
    BOOST_TEST(moved.vptr() == default_registry::static_vptr<std::string>);

    // assignment from a value sets it statically
    moved = Dog{"Snoopy"};
    BOOST_TEST(moved.vptr() == default_registry::static_vptr<Dog>);
    BOOST_TEST(name(moved) == "Snoopy the dog");

    // emplace constructs in place and sets it statically
    moved.emplace<std::string>("Sylvester");
    BOOST_TEST(moved.vptr() == default_registry::static_vptr<std::string>);
    BOOST_TEST(te::any_cast<const std::string&>(moved.get()) == "Sylvester");
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
    struct name_id, std::string(const virtual_any<erased, indirect_registry>&),
    indirect_registry>;

auto name_dog(const Dog& dog) -> std::string {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_REGISTER(name_method::override<name_dog>);

BOOST_AUTO_TEST_CASE(virtual_any_indirect_vptr) {
    initialize<indirect_registry>();

    erased spot_any(Dog{"Spot"});
    virtual_any<erased, indirect_registry> spot = spot_any;
    BOOST_TEST(spot.vptr() == indirect_registry::static_vptr<Dog>);
    BOOST_TEST(name_method::fn(spot) == "Spot the dog");

    virtual_any<erased, indirect_registry> rex = Dog{"Rex"};
    BOOST_TEST(name_method::fn(rex) == "Rex the dog");
}
} // namespace BOOST_OPENMETHOD_GENSYM
