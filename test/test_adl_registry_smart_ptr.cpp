// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// The affinity is found through a smart pointer, so the alias spellings and
// the `virtual_ptr` they stand for agree about the registry.

#include <memory>
#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/boost_intrusive_ptr.hpp>
#include <boost/openmethod/interop/std_shared_ptr.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>
#include <boost/openmethod/initialize.hpp>

#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#define BOOST_TEST_MODULE adl_registry_smart_ptr
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry::with<policies::runtime_checks> {};

namespace zoo {

struct Animal : boost::intrusive_ref_counter<Animal> {
    virtual ~Animal() = default;
};

auto boost_openmethod_registry(Animal*) -> zoo_registry;

struct Dog : Animal {};

} // namespace zoo

using zoo::Animal, zoo::Dog;

// the anchor is the pointee, whatever the wrapper
static_assert(
    std::is_same_v<registry_affinity<std::shared_ptr<Dog>>, zoo_registry>);
static_assert(
    std::is_same_v<registry_affinity<std::unique_ptr<Dog>>, zoo_registry>);
static_assert(
    std::is_same_v<registry_affinity<boost::intrusive_ptr<Dog>>, zoo_registry>);
static_assert(std::is_same_v<
              registry_affinity<const std::shared_ptr<Dog>&>, zoo_registry>);

// so the alias and the type it stands for are the same type
static_assert(std::is_same_v<
              shared_virtual_ptr<Dog>,
              virtual_ptr<std::shared_ptr<Dog>, zoo_registry>>);
static_assert(std::is_same_v<
              unique_virtual_ptr<Dog>,
              virtual_ptr<std::unique_ptr<Dog>, zoo_registry>>);
static_assert(std::is_same_v<
              boost_intrusive_virtual_ptr<Dog>,
              virtual_ptr<boost::intrusive_ptr<Dog>, zoo_registry>>);

BOOST_OPENMETHOD_CLASSES(Animal, Dog, zoo_registry);

BOOST_OPENMETHOD(name, (shared_virtual_ptr<Animal>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (shared_virtual_ptr<Dog>), std::string) {
    return "dog";
}

BOOST_AUTO_TEST_CASE(factories_need_no_registry_argument) {
    initialize<zoo_registry>();

    auto dog = make_shared_virtual<Dog>();
    static_assert(std::is_same_v<decltype(dog), shared_virtual_ptr<Dog>>);
    BOOST_TEST(name(dog) == "dog");

    auto owned = make_unique_virtual<Dog>();
    static_assert(std::is_same_v<decltype(owned), unique_virtual_ptr<Dog>>);
}
