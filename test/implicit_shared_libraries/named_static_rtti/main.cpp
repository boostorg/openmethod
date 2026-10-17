// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// named_static_rtti across an implicitly linked shared library built with
// hidden visibility - b2's default, and what BoostRoot.cmake sets - or across a
// Windows DLL.
//
// Like static_rtti, named_static_rtti's type_ids are addresses of static
// variables, and each module has its own copy: the library and the executable
// have different type_ids for the same class. With static_rtti, initialize()
// takes them for unrelated classes, and lays out each module's v-tables on its
// own: a call made in one module, on an object created in the other, lands in
// the wrong overrider, or reads past the end of a v-table. named_static_rtti
// maps the type_ids to the names of the classes, and initialize() merges them.

#define BOOST_TEST_MODULE implicit_shared_libraries_named_static_rtti

#include <boost/test/unit_test.hpp>

#include "lib.hpp"
#include "../../test_classes.hpp"

#include <boost/openmethod/initialize.hpp>

#include <string>

using namespace boost::openmethod;

BOOST_OPENMETHOD_TEST_CLASSES(Object, Animal, Dog, Cat);

namespace {

// Not the library's Stray, although the compiler prints the same name for both.
// This is the collision that every compiler leaves a mark for; local classes
// are marked only by GCC.
struct Stray : Animal {};

} // namespace

BOOST_OPENMETHOD_CLASSES(Animal, Stray);

// Registered by the executable; the library knows nothing about it.
BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Cat>), const char*) {
    return "meow";
}

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Stray>), const char*) {
    return "executable stray";
}

struct initialize_fixture {
    initialize_fixture() {
        initialize();
    }
};

BOOST_TEST_GLOBAL_FIXTURE(initialize_fixture);

BOOST_AUTO_TEST_CASE(one_class_across_modules) {
    BOOST_TEST(lib_registry_state_id() == static_registry::id());

    // The type_ids differ, which is what named_static_rtti copes with...
    auto exe_dog_type_id = static_registry::rtti::static_type<Dog>();
    BOOST_TEST(lib_dog_type_id() != exe_dog_type_id);

    // ...by mapping them to the same key.
    BOOST_TEST(
        (static_registry::rtti::type_index(lib_dog_type_id()) ==
            static_registry::rtti::type_index(exe_dog_type_id)));

    // One class, one v-table, whichever module created the object.
    auto lib_dog = lib_make_dog();
    auto exe_dog = make_unique_virtual<Dog>();
    BOOST_TEST(lib_dog.vptr() == exe_dog.vptr());
}

BOOST_AUTO_TEST_CASE(executable_dispatches_on_library_object) {
    // The library's speak takes slot 1, behind name, which is declared on a
    // base class, and the library lays out the v-tables. If the executable
    // had a speak of its own, it would take slot 0, and land in name.
    BOOST_TEST(std::string(speak(lib_make_dog())) == "woof");
}

BOOST_AUTO_TEST_CASE(library_dispatches_on_executable_object) {
    BOOST_TEST(std::string(lib_speak(make_unique_virtual<Dog>())) == "woof");
    BOOST_TEST(std::string(lib_speak(make_unique_virtual<Cat>())) == "meow");
    BOOST_TEST(std::string(speak(make_unique_virtual<Cat>())) == "meow");
    BOOST_TEST(std::string(lib_speak(make_unique_virtual<Animal>())) == "?");
}

BOOST_AUTO_TEST_CASE(classes_in_anonymous_namespaces_stay_apart) {
    auto lib_stray = lib_make_stray();
    auto exe_stray = make_unique_virtual<Stray>();

    BOOST_TEST(lib_stray.vptr() != exe_stray.vptr());
    BOOST_TEST(std::string(speak(lib_stray)) == "library stray");
    BOOST_TEST(std::string(lib_speak(lib_stray)) == "library stray");
    BOOST_TEST(std::string(speak(exe_stray)) == "executable stray");
    BOOST_TEST(std::string(lib_speak(exe_stray)) == "executable stray");
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
