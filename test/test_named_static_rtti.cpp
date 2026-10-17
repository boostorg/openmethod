// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

struct named_registry;
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY named_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/named_static_rtti.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>
#include <boost/openmethod/initialize.hpp>

struct named_registry :
    boost::openmethod::registry<
        boost::openmethod::policies::named_static_rtti> {};

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "test_classes.hpp"

#include <sstream>
#include <string>
#include <string_view>

struct Animal {};

struct Dog : Animal {};

struct Cat : Animal {};

namespace {

struct Stray : Animal {};

} // namespace

using namespace boost::openmethod::aliases;

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog, Cat);

BOOST_OPENMETHOD_CLASSES(Animal, Stray);

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>, std::ostream&), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog>, std::ostream& os), void) {
    os << "bark";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Cat>, std::ostream& os), void) {
    os << "hiss";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Stray>, std::ostream& os), void) {
    os << "growl";
}

using rtti = named_registry::rtti;

template<class Class>
auto key() {
    return rtti::type_index(rtti::static_type<Class>());
}

template<class Class>
auto name() -> std::string {
    std::ostringstream os;
    rtti::type_name(rtti::static_type<Class>(), os);

    return os.str();
}

BOOST_AUTO_TEST_CASE(names) {
    // The class-keys MSVC prints are dropped, so the name is the same on
    // every compiler.
    BOOST_TEST(name<Dog>() == "Dog");
    BOOST_TEST(name<Cat>() == "Cat");
}

// MSVC prints the class-key of every class it names; the other compilers
// print none. Only the strip itself can be checked here, on names spelled the
// way MSVC spells them.
BOOST_AUTO_TEST_CASE(class_keys) {
    using boost::openmethod::detail::static_rtti_strip;
    using boost::openmethod::detail::static_rtti_stripped_size;

#define TEST_STRIP(RAW, STRIPPED)                                              \
    {                                                                          \
        constexpr std::string_view raw = RAW;                                  \
        constexpr auto size = static_rtti_stripped_size(raw);                  \
        constexpr auto chars = static_rtti_strip<size>(raw);                   \
        BOOST_TEST(std::string_view(chars.data(), size) == STRIPPED);          \
    }

    TEST_STRIP("struct Dog", "Dog")
    TEST_STRIP("class Dog", "Dog")
    TEST_STRIP("union U", "U")
    TEST_STRIP("Dog", "Dog")
    TEST_STRIP("struct Box<struct Dog,enum Color>", "Box<Dog,Color>")
    TEST_STRIP("struct A<class B<union C>>", "A<B<C>>")
    TEST_STRIP("ns::class Y", "ns::Y")
    TEST_STRIP(
        "`anonymous-namespace'::struct Stray", "`anonymous-namespace'::Stray")

    // A class-key is only one where a name cannot continue.
    TEST_STRIP("structural", "structural")
    TEST_STRIP("enumerator", "enumerator")
    TEST_STRIP("struct Box<structural>", "Box<structural>")
    TEST_STRIP("myclass X", "myclass X")

#undef TEST_STRIP
}

BOOST_AUTO_TEST_CASE(keys) {
    BOOST_TEST((key<Dog>() == key<Dog>()));
    BOOST_TEST((key<Dog>() != key<Cat>()));
    BOOST_TEST((key<Dog>() < key<Cat>() || key<Cat>() < key<Dog>()));

    // A class at namespace scope is identified by its name alone...
    BOOST_TEST(key<Dog>().local == nullptr);

    // ...one in an anonymous namespace by its type_id as well, on every
    // compiler.
    BOOST_TEST(key<Stray>().local == rtti::static_type<Stray>());
}

// GCC prints both classes as `local_classes::test_method()::Local`, and marks
// them as local; reflection does not need marks. Clang and MSVC leave no mark:
// see the warning in the documentation of named_static_rtti.
#if BOOST_OPENMETHOD_HAS_REFLECTION || defined(BOOST_GCC)
BOOST_AUTO_TEST_CASE(local_classes) {
    struct Local {};
    auto first = key<Local>();

    {
        struct Local {};
        auto second = key<Local>();

        BOOST_TEST(first.local != nullptr);
        BOOST_TEST((first != second));
    }
}
#endif

BOOST_AUTO_TEST_CASE(dispatch) {
    boost::openmethod::initialize();

    auto poked = [](auto animal) {
        std::ostringstream os;
        poke(animal, os);

        return os.str();
    };

    BOOST_TEST(poked(make_unique_virtual<Dog>()) == "bark");
    BOOST_TEST(poked(make_unique_virtual<Cat>()) == "hiss");
    BOOST_TEST(poked(make_unique_virtual<Stray>()) == "growl");
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
