// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <utility>

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

// `relaxed` implies typeid_<>, on which typeid_of and any_cast - thus
// dispatch - rely; no explicit typeid_<> needed.
using Concept = boost::mpl::vector<te::copy_constructible<>, te::relaxed>;
using erased = te::any<Concept>;
using erased_ref = te::any<Concept, te::_self&>;
using erased_cref = te::any<Concept, const te::_self&>;

static_assert(detail::has_vptr<
              virtual_traits<const erased&, default_registry>, const erased&>);

// A registry spelled on the parameter names the same parameter as the default
// one, for the owning any by reference and for the any references.
static_assert(detail::validate_method_parameter<
              virtual_<const erased&, default_registry>, default_registry,
              void>::value);
static_assert(
    detail::validate_method_parameter<
        virtual_<erased&, default_registry>, default_registry, void>::value);
static_assert(
    detail::validate_method_parameter<
        virtual_<erased&&, default_registry>, default_registry, void>::value);
static_assert(
    detail::validate_method_parameter<
        virtual_<erased_ref, default_registry>, default_registry, void>::value);
static_assert(detail::validate_method_parameter<
              virtual_<erased_cref, default_registry>, default_registry,
              void>::value);

#define MAKE_CLASSES()                                                         \
    struct Dog {                                                               \
        std::string name;                                                      \
    };

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as const any& (const ref)

MAKE_CLASSES();

BOOST_OPENMETHOD(name, (virtual_<const erased&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(name, (const std::string& name), std::string) {
    return name;
}

// A catch-all overrider may take the `any` itself; the argument is passed
// through unchanged.
BOOST_OPENMETHOD_OVERRIDE(name, (const erased& arg), std::string) {
    return te::is_empty(arg) ? "nothing" : "something";
}

// `int` is registered because `weigh`'s overrider names it; it has no
// `name` overrider, so the catch-all applies to it.

BOOST_OPENMETHOD(weigh, (virtual_<const erased&>), int);

BOOST_OPENMETHOD_OVERRIDE(weigh, (const int& value), int) {
    return value;
}

BOOST_AUTO_TEST_CASE(type_erasure_by_const_ref) {
    initialize(trace());

    const erased spot(Dog{"Spot"});
    const erased felix(std::string{"Felix the cat"});
    const erased answer(42);

    BOOST_TEST(weigh(answer) == 42);

    BOOST_TEST(name(spot) == "Spot the dog");
    BOOST_TEST(name(felix) == "Felix the cat");
    // `int` is registered, but has no specific overrider: the catch-all,
    // registered for the root, applies
    BOOST_TEST(name(answer) == "something");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as any& (mutable ref)

MAKE_CLASSES();

BOOST_OPENMETHOD(bump, (virtual_<erased&>), std::string);

// BOOST_OPENMETHOD_OVERRIDE cannot express this: a temporary `any` binds
// to `const any&` and to `any&&`, but nothing binds to a mutable lvalue
// reference. Register directly via method<...>::override<Fn> instead -
// the primitive the macro itself expands to.

using bump_method =
    BOOST_OPENMETHOD_TYPE(bump, (virtual_<erased&>), std::string);

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

BOOST_AUTO_TEST_CASE(type_erasure_by_mutable_ref) {
    initialize(trace());

    erased spot(Dog{"Spot"});
    BOOST_TEST(bump(spot) == "Spot Jr. the dog");
    // the mutation is visible through the `any`
    BOOST_TEST(te::any_cast<const Dog&>(spot).name == "Spot Jr.");

    erased answer(41);
    BOOST_TEST(bump(answer) == "bumped");
    BOOST_TEST(te::any_cast<int>(answer) == 42);
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as any&& (xvalue ref)

MAKE_CLASSES();

BOOST_OPENMETHOD(steal, (virtual_<erased&&>), std::string);

// boost::type_erasure::any_cast has no rvalue overload; the trait moves
// the result of a mutable-reference cast, because the `any` owns its
// value.
BOOST_OPENMETHOD_OVERRIDE(steal, (Dog && dog), std::string) {
    Dog stolen(std::move(dog));
    return stolen.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(steal, (std::string && name), std::string) {
    std::string stolen(std::move(name));
    return stolen;
}

BOOST_AUTO_TEST_CASE(type_erasure_by_xvalue_ref) {
    initialize(trace());

    erased spot(Dog{"Spot"});
    BOOST_TEST(steal(std::move(spot)) == "Spot the dog");
    // the overrider moved the name out; the `any` still owns the Dog
    BOOST_TEST(!te::is_empty(spot));
    BOOST_TEST(te::any_cast<const Dog&>(spot).name == "");

    erased felix(std::string{"Felix the cat"});
    BOOST_TEST(steal(std::move(felix)) == "Felix the cat");
    BOOST_TEST(te::any_cast<const std::string&>(felix) == "");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as any<C, _self&> - the mutable any reference - by
// value

MAKE_CLASSES();

BOOST_OPENMETHOD(poke, (virtual_<erased_ref>), std::string);

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog & dog), std::string) {
    dog.name += "!";
    return dog.name;
}

BOOST_OPENMETHOD_OVERRIDE(poke, (int& value), std::string) {
    ++value;
    return "poked";
}

BOOST_AUTO_TEST_CASE(type_erasure_any_ref_by_value) {
    initialize(trace());

    // the any reference is a cheap handle; mutations reach the referents
    Dog snoopy{"Snoopy"};
    int count = 41;

    BOOST_TEST(poke(erased_ref(snoopy)) == "Snoopy!");
    BOOST_TEST(snoopy.name == "Snoopy!");

    BOOST_TEST(poke(erased_ref(count)) == "poked");
    BOOST_TEST(count == 42);
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as any<C, const _self&> - the const any reference -
// by value

MAKE_CLASSES();

BOOST_OPENMETHOD(name, (virtual_<erased_cref>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

// the catch-all receives a copy of the any reference - still a cheap handle
BOOST_OPENMETHOD_OVERRIDE(name, (erased_cref arg), std::string) {
    return te::is_empty(arg) ? "nothing" : "something";
}

// `int` is registered because `weigh`'s overrider names it; it has no
// `name` overrider, so the catch-all applies to it.

BOOST_OPENMETHOD(weigh, (virtual_<erased_cref>), int);

BOOST_OPENMETHOD_OVERRIDE(weigh, (const int& value), int) {
    return value;
}

BOOST_AUTO_TEST_CASE(type_erasure_const_any_ref_by_value) {
    initialize(trace());

    Dog snoopy{"Snoopy"};
    const int count = 42;

    BOOST_TEST(weigh(erased_cref(count)) == 42);

    BOOST_TEST(name(erased_cref(snoopy)) == "Snoopy the dog");
    BOOST_TEST(name(erased_cref(count)) == "something");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// indirect vptrs

struct Dog {
    std::string name;
};

// `Dog` is registered in `indirect_registry` - the method's registry - by
// the overrider

using name_method = method<
    struct name_id, std::string(virtual_<const erased&>), indirect_registry>;

auto name_dog(const Dog& dog) -> std::string {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_REGISTER(name_method::override<name_dog>);

BOOST_AUTO_TEST_CASE(type_erasure_indirect_vptr) {
    initialize<indirect_registry>();

    const erased spot(Dog{"Spot"});
    BOOST_TEST(name_method::fn(spot) == "Spot the dog");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// the openmethod_vptr concept: the any carries the v-table pointer for its
// bound type in its own dispatch table, and binding a value registers its type

struct Dog {
    std::string name;
};

// The concept must name the Concept it is part of: define the Concept as
// a struct.
struct Dispatchable :
    boost::mpl::vector<
        te::copy_constructible<>, te::relaxed, openmethod_vptr<Dispatchable>> {
};

using dispatchable = te::any<Dispatchable>;
using dispatchable_ref = te::any<Dispatchable, te::_self&>;

// the intrinsic hook is found for every any, so dispatch prefers it
// over the vptr policy's hash lookup
static_assert(detail::has_vptr_fn<dispatchable, default_registry>);
static_assert(detail::has_vptr_fn<dispatchable_ref, default_registry>);

// `Dog` is registered both by the `name` overrider below and by binding
// (class dedup)

BOOST_OPENMETHOD(name, (virtual_<const dispatchable&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(name, (const dispatchable& value), std::string) {
    return te::is_empty(value) ? "nothing" : "something";
}

BOOST_OPENMETHOD(poke, (virtual_<dispatchable_ref>), std::string);

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog & dog), std::string) {
    dog.name += "!";
    return dog.name;
}

BOOST_AUTO_TEST_CASE(type_erasure_openmethod_vptr_concept) {
    initialize(trace());

    const dispatchable spot(Dog{"Spot"});

    // the hook returns the static vptr for the bound type
    BOOST_TEST(
        boost_openmethod_vptr(spot, static_cast<default_registry*>(nullptr)) ==
        default_registry::static_vptr<Dog>);
    BOOST_TEST(name(spot) == "Spot the dog");

    // std::string appears nowhere in this section; binding it registered
    // it, and the catch-all applies
    const dispatchable felix(std::string{"Felix"});
    BOOST_TEST(name(felix) == "something");

    // the any reference takes the fast path too
    Dog snoopy{"Snoopy"};
    BOOST_TEST(poke(dispatchable_ref(snoopy)) == "Snoopy!");
    BOOST_TEST(snoopy.name == "Snoopy!");
}
} // namespace BOOST_OPENMETHOD_GENSYM

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// the openmethod_vptr concept, with indirect vptrs

struct Dog {
    std::string name;
};

struct Dispatchable :
    boost::mpl::vector<
        te::copy_constructible<>, te::relaxed,
        openmethod_vptr<Dispatchable, indirect_registry>> {};

using dispatchable = te::any<Dispatchable>;

// The hook is keyed on the registry the concept names: it is found for
// `indirect_registry`, and for no other - a mismatch would fall back on the
// vptr policy's hash lookup, silently losing the constant-time property.
static_assert(detail::has_vptr_fn<dispatchable, indirect_registry>);
static_assert(!detail::has_vptr_fn<dispatchable, default_registry>);

using name_method = method<
    struct name_id, std::string(virtual_<const dispatchable&>),
    indirect_registry>;

auto name_dog(const Dog& dog) -> std::string {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_REGISTER(name_method::override<name_dog>);

BOOST_AUTO_TEST_CASE(type_erasure_openmethod_vptr_indirect) {
    initialize<indirect_registry>();

    const dispatchable spot(Dog{"Spot"});
    BOOST_TEST(name_method::fn(spot) == "Spot the dog");

    // The concept needs no indirection of its own - unlike inplace_vptr,
    // which stores a `const vptr_type*` under this policy. `apply` reads
    // `static_vptr` on every call, so it tracks a re-initialize, which
    // rebuilds the v-tables and updates `static_vptr`.
    initialize<indirect_registry>();

    BOOST_TEST(
        boost_openmethod_vptr(spot, static_cast<indirect_registry*>(nullptr)) ==
        indirect_registry::static_vptr<Dog>);
    BOOST_TEST(name_method::fn(spot) == "Spot the dog");
}
} // namespace BOOST_OPENMETHOD_GENSYM
