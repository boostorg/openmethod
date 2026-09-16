// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// A class declares an affinity for a registry once, next to itself. Everything
// that mentions the class then finds that registry on its own: `virtual_ptr`,
// and the methods that take the class as a virtual parameter.

#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE adl_registry
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry::with<policies::runtime_checks> {};

namespace zoo {

struct Animal {
    virtual ~Animal() = default;
};

auto boost_openmethod_registry(Animal*) -> zoo_registry;

struct Dog : Animal {};
struct Cat : Animal {};

} // namespace zoo

// A class that never declared one keeps the default registry.
struct Widget {
    virtual ~Widget() = default;
};

using zoo::Animal, zoo::Dog, zoo::Cat;

static_assert(std::is_same_v<registry_affinity<Animal>, zoo_registry>);
static_assert(std::is_same_v<registry_affinity<Dog>, zoo_registry>);
static_assert(std::is_same_v<registry_affinity<Widget>, default_registry>);

// A polymorphic class that happens to define `element_type` is not a smart
// pointer, and keeps its own affinity.
struct Matrix {
    using element_type = double;
    virtual ~Matrix() = default;
    friend auto boost_openmethod_registry(Matrix*) -> zoo_registry;
};

static_assert(std::is_same_v<registry_affinity<Matrix>, zoo_registry>);

// cv-qualifiers on the return type are stripped.
struct Rock {
    virtual ~Rock() = default;
    friend auto boost_openmethod_registry(Rock*) -> const zoo_registry;
};

static_assert(std::is_same_v<registry_affinity<Rock>, zoo_registry>);

struct kennel_registry : default_registry {};

// A member typedef declares an affinity too. It is visible from the point it
// is declared, so a class can mention `virtual_ptr` of itself in its own body,
// where it is still incomplete.
struct Node {
    using boost_openmethod_registry = zoo_registry;
    virtual ~Node() = default;
    virtual_ptr<Node> next;
};

static_assert(std::is_same_v<registry_affinity<Node>, zoo_registry>);
static_assert(
    std::is_same_v<decltype(Node::next), virtual_ptr<Node, zoo_registry>>);

// Inherited like any member, and a derived class's hides the base's.
struct Leaf : Node {};
struct Twig : Node {
    using boost_openmethod_registry = kennel_registry;
};

static_assert(std::is_same_v<registry_affinity<Leaf>, zoo_registry>);
static_assert(std::is_same_v<registry_affinity<Twig>, kennel_registry>);

// The typedef takes precedence over an overload - here the one Animal's
// namespace declares, and Kennel inherits.
struct Kennel : Animal {
    using boost_openmethod_registry = kennel_registry;
};

static_assert(std::is_same_v<registry_affinity<Kennel>, kennel_registry>);

// A hidden friend that precedes the member is found too: the class is being
// defined, and lookup sees what has been declared so far.
struct Chain {
    virtual ~Chain() = default;
    friend auto boost_openmethod_registry(Chain*) -> zoo_registry;
    virtual_ptr<Chain> next;
};

static_assert(std::is_same_v<registry_affinity<Chain>, zoo_registry>);

// `virtual_ptr` picks it up, so `virtual_ptr<Dog>` is not a `virtual_ptr` in
// the default registry.
static_assert(std::is_same_v<virtual_ptr<Dog>, virtual_ptr<Dog, zoo_registry>>);
static_assert(
    std::is_same_v<virtual_ptr<Widget>, virtual_ptr<Widget, default_registry>>);

// The list names no registry: the classes agree on `zoo_registry`, so that is
// where they are registered. Listing it would say the same thing.
BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);

namespace {

template<class... Classes>
using picked = detail::class_list_registry<Classes...>;

// Every class declares the same registry - directly, or by inheriting the
// declaration.
static_assert(std::is_same_v<picked<Animal, Dog, Cat>, zoo_registry>);
static_assert(std::is_same_v<picked<Animal>, zoo_registry>);

// None declares one.
static_assert(std::is_same_v<picked<Widget>, default_registry>);
static_assert(std::is_same_v<picked<>, default_registry>);

// A registry listed explicitly wins, and a class that declares nothing goes
// along with it. A class that declares another does not - see
// compile_fail_classes_declared_mismatch.cpp - and neither does a list that
// mixes the two, see compile_fail_classes_mixed_affinities.cpp.
static_assert(
    std::is_same_v<picked<Animal, Dog, Widget, zoo_registry>, zoo_registry>);
static_assert(std::is_same_v<picked<Widget, kennel_registry>, kennel_registry>);

} // namespace

// Neither declaration names a registry; both land in `zoo_registry`.
BOOST_OPENMETHOD(speak, (virtual_<const Animal&>), std::string);
BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), std::string);

static_assert(
    std::is_same_v<
        BOOST_OPENMETHOD_TYPE(speak, (virtual_<const Animal&>), std::string),
        method<
            BOOST_OPENMETHOD_ID(speak), std::string(virtual_<const Animal&>),
            zoo_registry>>);

BOOST_OPENMETHOD_OVERRIDE(speak, (const Dog&), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (const Cat&), std::string) {
    return "meow";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog>), std::string) {
    return "woof";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Cat>), std::string) {
    return "hiss";
}

BOOST_AUTO_TEST_CASE(dispatch_in_the_registry_the_class_names) {
    initialize<zoo_registry>();

    Dog spot;
    Cat felix;

    BOOST_TEST(speak(spot) == "bark");
    BOOST_TEST(speak(felix) == "meow");
    BOOST_TEST(poke(virtual_ptr<Dog>(spot)) == "woof");
    BOOST_TEST(poke(virtual_ptr<Cat>(felix)) == "hiss");
}
