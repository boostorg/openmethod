// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Tests for reflection-based class registration - `register_classes` and
// `BOOST_OPENMETHOD_REGISTER_CLASSES`. Without a compiler that supports C++26
// reflection there is nothing to test, and the whole file reduces to one test
// case that says so.

#include <sstream>
#include <string>
#include <utility>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE reflection
#include <boost/test/unit_test.hpp>

#if !BOOST_OPENMETHOD_HAS_REFLECTION

BOOST_AUTO_TEST_CASE(reflection_not_supported) {
    BOOST_TEST_MESSAGE("compiler does not support C++26 reflection");
}

#else

#include <memory>

#include <boost/openmethod/interop/std_shared_ptr.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>

using namespace boost::openmethod;

// True if `Class` was registered in `Registry`, as seen by the compiler object
// `initialize` returns.
template<class Class, class Registry, class Compiler>
auto registered(const Compiler& comp) -> bool {
    return comp.class_map.find(
               Registry::rtti::type_index(
                   Registry::rtti::template static_type<Class>())) !=
        comp.class_map.end();
}

// =============================================================================
// The macro interface, and a leaf class no signature names

namespace macro_interface {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};
struct Cat : Animal {};

// Never named in a method or an overrider, and never wrapped in a virtual_ptr.
// Only the namespace scan can find it.
struct Bulldog : Dog {};

// Not related to any virtual parameter: must be left alone.
struct Fence {
    virtual ~Fence() = default;
};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (Cat&), std::string) {
    return "hiss";
}

} // namespace macro_interface

// Braces around a group of one are optional; the fixtures below use them.
BOOST_OPENMETHOD_REGISTER(
    register_classes<^^macro_interface, ^^macro_interface::test_registry>);

BOOST_AUTO_TEST_CASE(macro_interface_dispatch) {
    using namespace macro_interface;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));
    BOOST_TEST((registered<Cat, test_registry>(comp)));
    // Found by the scan, although nothing dispatches on it.
    BOOST_TEST((registered<Bulldog, test_registry>(comp)));
    // Unrelated to every virtual parameter.
    BOOST_TEST((!registered<Fence, test_registry>(comp)));

    Animal animal;
    Dog dog;
    Cat cat;
    Bulldog bulldog;

    BOOST_TEST(poke(animal) == "generic");
    BOOST_TEST(poke(dog) == "bark");
    BOOST_TEST(poke(cat) == "hiss");
    // Bulldog has no overrider of its own; it is registered as derived from
    // Dog, so Dog's overrider applies.
    BOOST_TEST(poke(bulldog) == "bark");
}

// =============================================================================
// The core interface: a method named by an alias, overriders as free functions

namespace core_interface {

struct test_registry : test_registry_<__COUNTER__> {};

struct Node {
    virtual ~Node() = default;
};

struct Literal : Node {};
struct Plus : Node {};

struct BOOST_OPENMETHOD_ID(value);

using value = method<
    BOOST_OPENMETHOD_ID(value), std::string(virtual_ptr<Node, test_registry>),
    test_registry>;

auto value_literal(virtual_ptr<Literal, test_registry>) -> std::string {
    return "literal";
}

auto value_plus(virtual_ptr<Plus, test_registry>) -> std::string {
    return "plus";
}

BOOST_OPENMETHOD_REGISTER(value::override<value_literal, value_plus>);

} // namespace core_interface

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^core_interface}, {^^core_interface::test_registry}>);

BOOST_AUTO_TEST_CASE(core_interface_dispatch) {
    using namespace core_interface;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Node, test_registry>(comp)));
    BOOST_TEST((registered<Literal, test_registry>(comp)));
    BOOST_TEST((registered<Plus, test_registry>(comp)));

    Literal literal;
    Plus plus;

    BOOST_TEST(
        value::fn(virtual_ptr<Node, test_registry>(literal)) == "literal");
    BOOST_TEST(value::fn(virtual_ptr<Node, test_registry>(plus)) == "plus");
}

// =============================================================================
// A method with no alias is found through an overrider registrar

namespace registrar_only {

struct test_registry : test_registry_<__COUNTER__> {};

struct Node {
    virtual ~Node() = default;
};

struct Literal : Node {};

struct value_id;

auto value_literal(virtual_ptr<Literal, test_registry>) -> std::string {
    return "literal";
}

// The method type is spelled out in full; the registrar variable below is the
// only namespace member whose type involves the specialization. The scan finds
// the method through it - `specialization_named_by` accepts a variable whose
// type is nested in a `method` specialization.
BOOST_OPENMETHOD_REGISTER(
    method<
        value_id, std::string(virtual_ptr<Node, test_registry>),
        test_registry>::override<value_literal>);

} // namespace registrar_only

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^registrar_only}, {^^registrar_only::test_registry}>);

BOOST_AUTO_TEST_CASE(method_without_alias_is_found_through_a_registrar) {
    using namespace registrar_only;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Node, test_registry>(comp)));
    BOOST_TEST((registered<Literal, test_registry>(comp)));

    Literal literal;

    using value = method<
        value_id, std::string(virtual_ptr<Node, test_registry>), test_registry>;
    BOOST_TEST(
        value::fn(virtual_ptr<Node, test_registry>(literal)) == "literal");
}

// =============================================================================
// A method with no overrider at all

namespace method_only {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

// No overrider, so no registrar object names the method. It is found through
// the alias BOOST_OPENMETHOD declares for it.
BOOST_OPENMETHOD(poke, (virtual_<Animal&>), void, test_registry);

} // namespace method_only

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^method_only}, {^^method_only::test_registry}>);

BOOST_AUTO_TEST_CASE(method_without_overrider) {
    using namespace method_only;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));
}

// =============================================================================
// Inheritance: virtual, multiple, and inaccessible bases

namespace inheritance {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Herbivore : virtual Animal {};
struct Carnivore : virtual Animal {};
struct Omnivore : Herbivore, Carnivore {};

// Reached only through a private base: the library cannot convert a Stowaway to
// an Animal, so it must not be registered as one.
struct Stowaway : private Animal {};

BOOST_OPENMETHOD(
    meet, (virtual_<Animal&>, virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(meet, (Animal&, Animal&), std::string) {
    return "ignore";
}

BOOST_OPENMETHOD_OVERRIDE(meet, (Carnivore&, Herbivore&), std::string) {
    return "hunt";
}

} // namespace inheritance

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^inheritance}, {^^inheritance::test_registry}>);

BOOST_AUTO_TEST_CASE(virtual_and_multiple_inheritance) {
    using namespace inheritance;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Herbivore, test_registry>(comp)));
    BOOST_TEST((registered<Carnivore, test_registry>(comp)));
    BOOST_TEST((registered<Omnivore, test_registry>(comp)));
    BOOST_TEST((!registered<Stowaway, test_registry>(comp)));

    Herbivore herbivore;
    Carnivore carnivore;
    Omnivore omnivore;

    BOOST_TEST(meet(herbivore, herbivore) == "ignore");
    BOOST_TEST(meet(carnivore, herbivore) == "hunt");
    // Omnivore is both, and inherits the Carnivore/Herbivore overrider.
    BOOST_TEST(meet(omnivore, omnivore) == "hunt");
}

// =============================================================================
// Repeated inheritance is left out, not rejected
//
// `use_classes` rejects an ambiguous base at compile time, because naming one
// is a mistake in a hand-written list. Here the classes are collected
// mechanically, and a class that happens to have one must not break the build:
// it is registered under the bases it can be converted to, and the ambiguous
// one is left out of its list.

namespace repeated_inheritance {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

struct Left : Animal {};
struct Right : Animal {};
// Animal is an ambiguous base: no conversion to it exists.
struct Repeated : Left, Right {};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

} // namespace repeated_inheritance

BOOST_OPENMETHOD_REGISTER(
    register_classes<
        {^^repeated_inheritance}, {^^repeated_inheritance::test_registry}>);

BOOST_AUTO_TEST_CASE(repeated_inheritance_does_not_break_the_scan) {
    using namespace repeated_inheritance;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));
    BOOST_TEST((registered<Left, test_registry>(comp)));
    BOOST_TEST((registered<Right, test_registry>(comp)));
    // Registered, under Left and Right: Animal is ambiguous, and left out.
    BOOST_TEST((registered<Repeated, test_registry>(comp)));

    auto repeated = comp.class_map.at(
        test_registry::rtti::type_index(
            test_registry::rtti::static_type<Repeated>()));
    BOOST_TEST(repeated->direct_bases.size() == 2u);

    Dog dog;
    Left left;
    BOOST_TEST(poke(dog) == "bark");
    BOOST_TEST(poke(left) == "generic");

    // A Repeated cannot be passed as an Animal, but a Left can, and its dynamic
    // type is what dispatch looks at.
    Repeated repeated_obj;
    Left& left_side = repeated_obj;
    BOOST_TEST(poke(left_side) == "generic");
}

// =============================================================================
// Nested namespaces, smart pointers, and covariant return types

namespace nested {

struct test_registry : test_registry_<__COUNTER__> {};

namespace shapes {

struct Shape {
    virtual ~Shape() = default;
};

namespace round {
struct Circle : Shape {};
} // namespace round

} // namespace shapes

BOOST_OPENMETHOD(
    name, (virtual_ptr<std::shared_ptr<shapes::Shape>, test_registry>),
    std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(
    name, (virtual_ptr<std::shared_ptr<shapes::round::Circle>, test_registry>),
    std::string) {
    return "circle";
}

} // namespace nested

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^nested}, {^^nested::test_registry}>);

BOOST_AUTO_TEST_CASE(nested_namespaces_and_smart_pointers) {
    using namespace nested;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<shapes::Shape, test_registry>(comp)));
    BOOST_TEST((registered<shapes::round::Circle, test_registry>(comp)));

    std::shared_ptr<shapes::Shape> circle =
        std::make_shared<shapes::round::Circle>();
    BOOST_TEST(
        name(
            virtual_ptr<std::shared_ptr<shapes::Shape>, test_registry>(
                circle)) == "circle");
}

// =============================================================================
// A base class nothing dispatches on is not registered
//
// Registering it would cost a class_info, a perfect-hash slot and dispatch table
// space, for a class no overrider can ever be selected on.

namespace unused_bases {

struct test_registry : test_registry_<__COUNTER__> {};

// Neither of these is a virtual parameter of any method below.
struct Serializable {
    virtual ~Serializable() = default;
};

struct Named : Serializable {};

struct Animal : Named {};
struct Dog : Animal {};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

} // namespace unused_bases

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^unused_bases}, {^^unused_bases::test_registry}>);

BOOST_AUTO_TEST_CASE(bases_that_take_no_part_in_dispatch_are_left_out) {
    using namespace unused_bases;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));
    // Above the only virtual parameter, so of no use to dispatch.
    BOOST_TEST((!registered<Named, test_registry>(comp)));
    BOOST_TEST((!registered<Serializable, test_registry>(comp)));

    Animal animal;
    Dog dog;
    BOOST_TEST(poke(animal) == "generic");
    BOOST_TEST(poke(dog) == "bark");
}

// =============================================================================
// ... unless another method dispatches on it

namespace shared_bases {

struct test_registry : test_registry_<__COUNTER__> {};

struct Serializable {
    virtual ~Serializable() = default;
};

struct Named : Serializable {};

struct Animal : Named {};
struct Dog : Animal {};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

// Named is a virtual parameter here, so it - and the lattice edges through it -
// must be registered after all.
BOOST_OPENMETHOD(label, (virtual_<Named&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(label, (Named&), std::string) {
    return "named";
}

BOOST_OPENMETHOD_OVERRIDE(label, (Dog&), std::string) {
    return "dog";
}

} // namespace shared_bases

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^shared_bases}, {^^shared_bases::test_registry}>);

BOOST_AUTO_TEST_CASE(a_base_another_method_dispatches_on_is_registered) {
    using namespace shared_bases;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Named, test_registry>(comp)));
    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));
    // Still above every virtual parameter.
    BOOST_TEST((!registered<Serializable, test_registry>(comp)));

    Animal animal;
    Dog dog;
    BOOST_TEST(poke(dog) == "bark");
    // Dog reaches Named through Animal: the edges survive.
    BOOST_TEST(label(dog) == "dog");
    BOOST_TEST(label(animal) == "named");
}

// =============================================================================
// The recorded bases are every registered ancestor
//
// Not the direct bases only: `initialize` derives those from the closure of
// what the records say, and a record that carries the whole ancestry stands on
// its own, whatever another registration - over other namespaces, in another
// translation unit - records alongside it.

namespace direct_bases {

struct test_registry : test_registry_<__COUNTER__> {};

struct A {
    virtual ~A() = default;
};

struct B : A {};
struct C : B {};
struct D : C {};

BOOST_OPENMETHOD(poke, (virtual_<A&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (A&), std::string) {
    return "A";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (C&), std::string) {
    return "C";
}

} // namespace direct_bases

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^direct_bases}, {^^direct_bases::test_registry}>);

BOOST_AUTO_TEST_CASE(recorded_bases_are_every_registered_ancestor) {
    using namespace direct_bases;

    auto comp = initialize<test_registry>();

    // Each class_info names the class itself, as its own improper base, plus
    // every registered class above it.
    std::size_t recorded = 0;

    for (auto& ci : test_registry::state().classes) {
        recorded += ci.last_base - ci.first_base;
    }

    BOOST_TEST(recorded == 10u); // A: 1, B: 2, C: 3, D: 4

    // The lattice initialize derives from them is the chain, with the direct
    // bases worked out.
    auto a = comp.class_map.at(
        test_registry::rtti::type_index(test_registry::rtti::static_type<A>()));
    auto d = comp.class_map.at(
        test_registry::rtti::type_index(test_registry::rtti::static_type<D>()));

    BOOST_TEST(d->direct_bases.size() == 1u);
    BOOST_TEST(d->transitive_bases.size() == 3u);
    BOOST_TEST(a->transitive_derived.size() == 4u);

    A a_obj;
    B b;
    C c;
    D d_obj;
    BOOST_TEST(poke(a_obj) == "A");
    BOOST_TEST(poke(b) == "A");
    BOOST_TEST(poke(c) == "C");
    BOOST_TEST(poke(d_obj) == "C");
}

// =============================================================================
// Several namespaces in one registration

namespace two_namespaces {

struct test_registry : test_registry_<__COUNTER__> {};

namespace zoo {

struct Animal {
    virtual ~Animal() = default;
};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

} // namespace zoo

namespace pets {

struct Dog : zoo::Animal {};

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

} // namespace pets

} // namespace two_namespaces

BOOST_OPENMETHOD_REGISTER(
    register_classes<
        {^^two_namespaces::zoo, ^^two_namespaces::pets},
        {^^two_namespaces::test_registry}>);

BOOST_AUTO_TEST_CASE(several_namespaces_in_one_registration) {
    using namespace two_namespaces;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<zoo::Animal, test_registry>(comp)));
    BOOST_TEST((registered<pets::Dog, test_registry>(comp)));

    pets::Dog dog;
    BOOST_TEST(zoo::poke(dog) == "bark");
}

// =============================================================================
// With no namespace group, the global namespace is scanned

namespace enclosing_macro {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

namespace elsewhere {

// Only the registry is named, and the registrar sits in a namespace holding
// neither the method nor the classes: what is scanned is `^^::`.
BOOST_OPENMETHOD_REGISTER_CLASSES(^^enclosing_macro::test_registry);

} // namespace elsewhere

} // namespace enclosing_macro

BOOST_AUTO_TEST_CASE(macro_scans_the_global_namespace) {
    using namespace enclosing_macro;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));

    Dog dog;
    BOOST_TEST(poke(dog) == "bark");
}

namespace enclosing_template {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

namespace elsewhere {

// Same, without the macro.
BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^enclosing_template::test_registry}>);

} // namespace elsewhere

} // namespace enclosing_template

BOOST_AUTO_TEST_CASE(template_scans_the_global_namespace) {
    using namespace enclosing_template;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));

    Dog dog;
    BOOST_TEST(poke(dog) == "bark");
}

// =============================================================================
// Classes without a namespace root a scan of the global namespace

namespace classes_only {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};
struct Bulldog : Dog {};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

} // namespace classes_only

// Naming no namespace does not turn the scan off: `Animal` roots a scan of
// `^^::`, which finds `Dog` and `Bulldog` under it.
BOOST_OPENMETHOD_REGISTER(
    register_classes<
        {^^classes_only::Animal}, {^^classes_only::test_registry}>);

BOOST_AUTO_TEST_CASE(listed_classes_root_a_scan_of_the_global_namespace) {
    using namespace classes_only;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));
    BOOST_TEST((registered<Bulldog, test_registry>(comp)));

    Bulldog snoopy;
    BOOST_TEST(poke(snoopy) == "generic");
}

// =============================================================================
// A listed class is a root for the scan

namespace listed_root {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

// No method dispatches on `Tool`; only listing it registers it - along with
// `Hammer`, which the scan finds derives from it.
struct Tool {
    virtual ~Tool() = default;
};

struct Hammer : Tool {};

// Unrelated to any root: must be left alone, as always.
struct Fence {
    virtual ~Fence() = default;
};

} // namespace listed_root

BOOST_OPENMETHOD_REGISTER(
    register_classes<
        ^^listed_root, ^^listed_root::Tool, ^^listed_root::test_registry>);

BOOST_AUTO_TEST_CASE(a_listed_class_roots_the_scan) {
    using namespace listed_root;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Tool, test_registry>(comp)));
    BOOST_TEST((registered<Hammer, test_registry>(comp)));
    BOOST_TEST((!registered<Fence, test_registry>(comp)));
}

// =============================================================================
// boost is skipped by a recursive scan, and reached by listing it

namespace boost_gate {

struct default_registry_ : test_registry_<__COUNTER__> {};
struct listed_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, default_registry_);
BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

BOOST_OPENMETHOD(poke2, (virtual_<Animal&>), std::string, listed_registry);
BOOST_OPENMETHOD_OVERRIDE(poke2, (Animal&), std::string) {
    return "generic";
}

} // namespace boost_gate

namespace boost::om_reflection_test {

struct Stray : boost_gate::Animal {};

} // namespace boost::om_reflection_test

// An alias, at global scope, to the very namespace the scan stays out of. The
// scan does not go through aliases, so this one changes nothing.
namespace om_reflection_test_alias = boost::om_reflection_test;

// The default scan of `^^::` recurses everywhere except into `std` and
// `boost`, so it does not reach `Stray`.
BOOST_OPENMETHOD_REGISTER(register_classes<{^^boost_gate::default_registry_}>);

// Listing a namespace always scans it, `boost` and its nested namespaces
// included. `boost_gate` is listed too, as that is where the method is.
BOOST_OPENMETHOD_REGISTER(
    register_classes<
        {^^boost_gate, ^^boost::om_reflection_test},
        {^^boost_gate::listed_registry}>);

BOOST_AUTO_TEST_CASE(boost_is_scanned_only_when_listed) {
    using namespace boost_gate;
    using boost::om_reflection_test::Stray;

    auto default_comp = initialize<default_registry_>();
    BOOST_TEST((registered<Animal, default_registry_>(default_comp)));
    BOOST_TEST((!registered<Stray, default_registry_>(default_comp)));

    auto listed_comp = initialize<listed_registry>();
    BOOST_TEST((registered<Animal, listed_registry>(listed_comp)));
    BOOST_TEST((registered<Stray, listed_registry>(listed_comp)));
}

// =============================================================================
// Several registries in one registration

namespace two_registries {

struct first_registry : test_registry_<__COUNTER__> {};
struct second_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, first_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

} // namespace two_registries

BOOST_OPENMETHOD_REGISTER(
    register_classes<
        {^^two_registries::Animal, ^^two_registries::Dog},
        {^^two_registries::first_registry, ^^two_registries::second_registry}>);

BOOST_AUTO_TEST_CASE(several_registries_in_one_registration) {
    using namespace two_registries;

    auto first_comp = initialize<first_registry>();
    BOOST_TEST((registered<Animal, first_registry>(first_comp)));
    BOOST_TEST((registered<Dog, first_registry>(first_comp)));

    auto second_comp = initialize<second_registry>();
    BOOST_TEST((registered<Animal, second_registry>(second_comp)));
    BOOST_TEST((registered<Dog, second_registry>(second_comp)));

    Dog dog;
    BOOST_TEST(poke(dog) == "bark");
}

// =============================================================================
// No registry: the default one

namespace default_registry_target {

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

// No registry group, so the classes go to the default registry; no namespace
// group either, so the global namespace is scanned, as always.
BOOST_OPENMETHOD_REGISTER(
    register_classes<{
        ^^default_registry_target::Animal, ^^default_registry_target::Dog}>);

} // namespace default_registry_target

BOOST_AUTO_TEST_CASE(no_registry_group_means_the_default_registry) {
    using namespace default_registry_target;

    auto comp = initialize<default_registry>();

    BOOST_TEST((registered<Animal, default_registry>(comp)));
    BOOST_TEST((registered<Dog, default_registry>(comp)));
}

// =============================================================================
// Classes nested in classes

namespace nested_classes {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

// The scan descends into a class as it does into a namespace, whatever the
// access of what it finds: `Kennel::Dog` takes part in dispatch through
// `Animal&`, and never has to be named from outside.
struct Kennel {
    struct Dog : Animal {};

  private:
    struct Puppy : Dog {};

  public:
    static auto make_puppy() -> std::unique_ptr<Animal> {
        return std::make_unique<Puppy>();
    }
};

// An alias to a class the scan declares anyway adds nothing, and is not walked
// into: Kennel::Dog is found once. An alias that adds cv-qualification names
// the same class too, not a second one to give a lattice node, a hash slot and
// a dispatch table row of its own.
using Alias = Kennel;
using ConstDog = const Kennel::Dog;

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (Kennel::Dog&), std::string) {
    return "bark";
}

} // namespace nested_classes

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^nested_classes}, {^^nested_classes::test_registry}>);

BOOST_AUTO_TEST_CASE(classes_nested_in_classes_are_found) {
    using namespace nested_classes;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Kennel::Dog, test_registry>(comp)));

    Animal animal;
    Kennel::Dog dog;
    BOOST_TEST(poke(animal) == "generic");
    BOOST_TEST(poke(dog) == "bark");
    // Puppy is private to Kennel, and reached only as an Animal.
    BOOST_TEST(poke(*Kennel::make_puppy()) == "bark");
}

// =============================================================================
// Namespace aliases are not followed

namespace namespace_aliases {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

namespace detail {

// An alias to the enclosing namespace: a scan that followed it would never
// end.
namespace up = ::namespace_aliases;

} // namespace detail

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

} // namespace namespace_aliases

BOOST_OPENMETHOD_REGISTER(
    register_classes<
        {^^namespace_aliases}, {^^namespace_aliases::test_registry}>);

BOOST_AUTO_TEST_CASE(namespace_aliases_are_not_followed) {
    using namespace namespace_aliases;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));

    Dog dog;
    BOOST_TEST(poke(dog) == "bark");
}

// =============================================================================
// An alias scanned before the declaration does not hide nested classes
//
// `early` is scanned first, and names `Kennel`. That must not stop the scan
// from walking into `Kennel` when it reaches `late`, which declares it.

namespace alias_first {

struct test_registry : test_registry_<__COUNTER__> {};

namespace late {
struct Kennel;
}

namespace early {
using K = late::Kennel;
}

namespace late {

struct Animal {
    virtual ~Animal() = default;
};

struct Kennel {
    struct Dog : Animal {};
};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Kennel::Dog&), std::string) {
    return "bark";
}

} // namespace late

} // namespace alias_first

BOOST_OPENMETHOD_REGISTER(
    register_classes<
        {^^alias_first::early, ^^alias_first::late},
        {^^alias_first::test_registry}>);

BOOST_AUTO_TEST_CASE(an_alias_scanned_first_does_not_hide_nested_classes) {
    using namespace alias_first;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<late::Kennel::Dog, test_registry>(comp)));

    late::Kennel::Dog dog;
    BOOST_TEST(late::poke(dog) == "bark");
}

// =============================================================================
// A class between a registered class and a root is registered wherever it is
//
// Two registrations, each scanning its own namespaces - as a library and a
// plugin would, from their own translation units. The plugin's class derives
// from a class the plugin's scan never sees. Its record must carry it all the
// same, and the class must be registered, or the library's overrider for it
// would not apply to the plugin's class.

namespace split {

struct test_registry : test_registry_<__COUNTER__> {};

namespace core {

struct Animal {
    virtual ~Animal() = default;
};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);
BOOST_OPENMETHOD(name, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

BOOST_OPENMETHOD_OVERRIDE(name, (Animal&), std::string) {
    return "animal";
}

} // namespace core

namespace lib {

struct Dog : core::Animal {};

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

} // namespace lib

namespace plugin {

struct Bulldog : lib::Dog {};

// The plugin overrides `name` only. Its registrar is what leads the plugin's
// scan to the method, and to `Animal` as a root.
BOOST_OPENMETHOD_OVERRIDE(name, (Bulldog&), std::string) {
    return "bulldog";
}

} // namespace plugin

} // namespace split

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^split::core, ^^split::lib}, {^^split::test_registry}>);
BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^split::plugin}, {^^split::test_registry}>);

BOOST_AUTO_TEST_CASE(a_base_outside_the_scan_is_registered) {
    using namespace split;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<plugin::Bulldog, test_registry>(comp)));

    auto bulldog = comp.class_map.at(
        test_registry::rtti::type_index(
            test_registry::rtti::static_type<plugin::Bulldog>()));
    BOOST_TEST(bulldog->direct_bases.size() == 1u);
    BOOST_TEST(bulldog->transitive_bases.size() == 2u);

    plugin::Bulldog bulldog_obj;
    // Dog's overrider, not Animal's: the edge to Dog survived.
    BOOST_TEST(core::poke(bulldog_obj) == "bark");
    BOOST_TEST(core::name(bulldog_obj) == "bulldog");
}

// =============================================================================
// A method's return type is not a root
//
// `stream` returns a class reference, and its overrider a reference to a
// derived class - a covariant return type, both in a namespace the scan does
// not enter. Were `std::ostream` a root, `initialize` would want
// `std::ostringstream` registered as well, and abort. `clone` shows the
// covariant check still armed when the return classes are in the scan.

namespace return_types {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

BOOST_OPENMETHOD(stream, (virtual_<Animal&>), std::ostream&, test_registry);

BOOST_OPENMETHOD_OVERRIDE(stream, (Dog&), std::ostringstream&) {
    static std::ostringstream os;
    return os;
}

BOOST_OPENMETHOD(clone, (virtual_<Animal&>), Animal*, test_registry);

BOOST_OPENMETHOD_OVERRIDE(clone, (Dog & dog), Dog*) {
    return &dog;
}

} // namespace return_types

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^return_types}, {^^return_types::test_registry}>);

BOOST_AUTO_TEST_CASE(a_return_type_is_not_a_root) {
    using namespace return_types;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Animal, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));
    BOOST_TEST((!registered<std::ostream, test_registry>(comp)));

    Dog dog;
    stream(dog) << "bark";
    BOOST_TEST(static_cast<std::ostringstream&>(stream(dog)).str() == "bark");
    BOOST_TEST(clone(dog) == &dog);
}

// =============================================================================
// A specialization of a class template between a class and a root
//
// `members_of` yields the template, not its specializations, so the scan cannot
// find `Pet<int>` on its own. It is in the base list of `Dog`, and that is how
// it is registered.

namespace template_bases {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

template<class T>
struct Pet : Animal {};

struct Dog : Pet<int> {};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (Pet<int>&), std::string) {
    return "pet";
}

} // namespace template_bases

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^template_bases}, {^^template_bases::test_registry}>);

BOOST_AUTO_TEST_CASE(a_specialization_in_a_base_list_is_registered) {
    using namespace template_bases;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Pet<int>, test_registry>(comp)));
    BOOST_TEST((registered<Dog, test_registry>(comp)));

    Dog dog;
    BOOST_TEST(poke(dog) == "pet");
}

// =============================================================================
// An alias to a specialization over an incomplete class
//
// Asking whether `Edge` or `NodeBox` is a complete type would instantiate the
// specialization, which fails: `Node` is never defined. The scan does not ask.

namespace incomplete_aliases {

struct test_registry : test_registry_<__COUNTER__> {};

struct Node;

using Edge = std::pair<Node, Node>;

template<class T>
struct Box {
    T value;
};

using NodeBox = Box<Node>;

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog&), std::string) {
    return "bark";
}

} // namespace incomplete_aliases

BOOST_OPENMETHOD_REGISTER(
    register_classes<
        {^^incomplete_aliases}, {^^incomplete_aliases::test_registry}>);

BOOST_AUTO_TEST_CASE(an_alias_to_a_specialization_is_not_instantiated) {
    using namespace incomplete_aliases;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<Dog, test_registry>(comp)));

    Dog dog;
    BOOST_TEST(poke(dog) == "bark");
}

// =============================================================================
// Deep and wide hierarchies stay within the compiler's budget
//
// A chain of sixty classes, and two hundred classes deriving from one root,
// under the default -fconstexpr-ops-limit. The cost of the scan and of the
// records must be linear in the number of classes, give or take the depth.

namespace budget {

struct test_registry : test_registry_<__COUNTER__> {};

struct Animal {
    virtual ~Animal() = default;
};

using C0 = Animal;

#define BOOST_OPENMETHOD_TEST_CHAIN_LINK(z, n, _)                              \
    struct BOOST_PP_CAT(C, BOOST_PP_INC(n)) : BOOST_PP_CAT(C, n) {};

BOOST_PP_REPEAT(60, BOOST_OPENMETHOD_TEST_CHAIN_LINK, _)

#undef BOOST_OPENMETHOD_TEST_CHAIN_LINK

#define BOOST_OPENMETHOD_TEST_WIDE_LEAF(z, n, _)                               \
    struct BOOST_PP_CAT(W, n) : Animal {};

BOOST_PP_REPEAT(200, BOOST_OPENMETHOD_TEST_WIDE_LEAF, _)

#undef BOOST_OPENMETHOD_TEST_WIDE_LEAF

BOOST_OPENMETHOD(poke, (virtual_<Animal&>), std::string, test_registry);

BOOST_OPENMETHOD_OVERRIDE(poke, (Animal&), std::string) {
    return "generic";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (C30&), std::string) {
    return "C30";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (W199&), std::string) {
    return "W199";
}

} // namespace budget

BOOST_OPENMETHOD_REGISTER(
    register_classes<{^^budget}, {^^budget::test_registry}>);

BOOST_AUTO_TEST_CASE(deep_and_wide_hierarchies_are_within_budget) {
    using namespace budget;

    auto comp = initialize<test_registry>();

    BOOST_TEST((registered<C60, test_registry>(comp)));
    BOOST_TEST((registered<W0, test_registry>(comp)));
    BOOST_TEST((registered<W199, test_registry>(comp)));

    auto c60 = comp.class_map.at(
        test_registry::rtti::type_index(
            test_registry::rtti::static_type<C60>()));
    BOOST_TEST(c60->direct_bases.size() == 1u);
    BOOST_TEST(c60->transitive_bases.size() == 60u);

    C60 c60_obj;
    C29 c29;
    W199 w199;
    BOOST_TEST(poke(c60_obj) == "C30");
    BOOST_TEST(poke(c29) == "generic");
    BOOST_TEST(poke(w199) == "W199");
}

#endif
