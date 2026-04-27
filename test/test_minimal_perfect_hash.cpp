// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <set>

#define BOOST_TEST_MODULE minimal_perfect_hash
#include <boost/test/unit_test.hpp>

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/minimal_perfect_hash.hpp>
#include <boost/openmethod/policies/std_rtti.hpp>
#include <boost/openmethod/policies/vptr_vector.hpp>
#include <boost/openmethod/policies/stderr_output.hpp>
#include <boost/openmethod/policies/default_error_handler.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

using namespace boost::openmethod;
using namespace boost::openmethod::policies;

// Test registry with minimal_perfect_hash
struct minimal_hash_registry
    : registry<
          std_rtti, vptr_vector, minimal_perfect_hash,
          default_error_handler, stderr_output> {
};

// Test registry with runtime checks
struct minimal_hash_registry_with_checks
    : registry<
          std_rtti, vptr_vector, minimal_perfect_hash,
          default_error_handler, stderr_output, runtime_checks> {
};

namespace test_basic {

struct Animal {
    virtual ~Animal() {}
};

struct Dog : Animal {};
struct Cat : Animal {};
struct Bird : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat, Bird, minimal_hash_registry);

BOOST_OPENMETHOD(get_sound, (virtual_<const Animal&>), std::string, minimal_hash_registry);

BOOST_OPENMETHOD_OVERRIDE(get_sound, (const Dog&), std::string) {
    return "woof";
}

BOOST_OPENMETHOD_OVERRIDE(get_sound, (const Cat&), std::string) {
    return "meow";
}

BOOST_OPENMETHOD_OVERRIDE(get_sound, (const Bird&), std::string) {
    return "chirp";
}

BOOST_AUTO_TEST_CASE(basic_functionality) {
    initialize<minimal_hash_registry>();

    Dog dog;
    Cat cat;
    Bird bird;

    BOOST_TEST(get_sound(dog) == "woof");
    BOOST_TEST(get_sound(cat) == "meow");
    BOOST_TEST(get_sound(bird) == "chirp");
}

} // namespace test_basic

namespace test_hash_properties {

struct Base {
    virtual ~Base() {}
};

struct D1 : Base {};
struct D2 : Base {};
struct D3 : Base {};
struct D4 : Base {};
struct D5 : Base {};

BOOST_OPENMETHOD_CLASSES(Base, D1, D2, D3, D4, D5, minimal_hash_registry);

BOOST_OPENMETHOD(get_id, (virtual_<const Base&>), int, minimal_hash_registry);

BOOST_OPENMETHOD_OVERRIDE(get_id, (const D1&), int) {
    return 1;
}

BOOST_OPENMETHOD_OVERRIDE(get_id, (const D2&), int) {
    return 2;
}

BOOST_OPENMETHOD_OVERRIDE(get_id, (const D3&), int) {
    return 3;
}

BOOST_OPENMETHOD_OVERRIDE(get_id, (const D4&), int) {
    return 4;
}

BOOST_OPENMETHOD_OVERRIDE(get_id, (const D5&), int) {
    return 5;
}

BOOST_AUTO_TEST_CASE(minimal_hash_properties) {
    initialize<minimal_hash_registry>();

    // Test that all classes are correctly hashed
    D1 d1;
    D2 d2;
    D3 d3;
    D4 d4;
    D5 d5;

    BOOST_TEST(get_id(d1) == 1);
    BOOST_TEST(get_id(d2) == 2);
    BOOST_TEST(get_id(d3) == 3);
    BOOST_TEST(get_id(d4) == 4);
    BOOST_TEST(get_id(d5) == 5);

    // Verify that the hash function produces a minimal perfect hash
    // (This is implicit - if it didn't, initialization would fail or we'd get wrong results)
}

} // namespace test_hash_properties

namespace test_with_runtime_checks {

struct Vehicle {
    virtual ~Vehicle() {}
};

struct Car : Vehicle {};
struct Bike : Vehicle {};

BOOST_OPENMETHOD_CLASSES(Vehicle, Car, Bike, minimal_hash_registry_with_checks);

BOOST_OPENMETHOD(get_wheels, (virtual_<const Vehicle&>), int, minimal_hash_registry_with_checks);

BOOST_OPENMETHOD_OVERRIDE(get_wheels, (const Car&), int) {
    return 4;
}

BOOST_OPENMETHOD_OVERRIDE(get_wheels, (const Bike&), int) {
    return 2;
}

BOOST_AUTO_TEST_CASE(runtime_checks) {
    initialize<minimal_hash_registry_with_checks>();

    Car car;
    Bike bike;

    BOOST_TEST(get_wheels(car) == 4);
    BOOST_TEST(get_wheels(bike) == 2);
}

} // namespace test_with_runtime_checks

namespace test_empty {

struct Empty {
    virtual ~Empty() {}
};

BOOST_OPENMETHOD_CLASSES(Empty, minimal_hash_registry);

BOOST_OPENMETHOD(process, (virtual_<const Empty&>), int, minimal_hash_registry);

BOOST_OPENMETHOD_OVERRIDE(process, (const Empty&), int) {
    return 42;
}

BOOST_AUTO_TEST_CASE(single_class) {
    initialize<minimal_hash_registry>();

    Empty e;
    BOOST_TEST(process(e) == 42);
}

} // namespace test_empty

namespace test_large_hierarchy {

struct Root {
    virtual ~Root() {}
};

struct L1_1 : Root {};
struct L1_2 : Root {};
struct L1_3 : Root {};
struct L1_4 : Root {};
struct L1_5 : Root {};
struct L1_6 : Root {};
struct L1_7 : Root {};
struct L1_8 : Root {};
struct L1_9 : Root {};
struct L1_10 : Root {};

BOOST_OPENMETHOD_CLASSES(Root, L1_1, L1_2, L1_3, L1_4, L1_5, L1_6, L1_7, L1_8, L1_9, L1_10, minimal_hash_registry);

BOOST_OPENMETHOD(classify, (virtual_<const Root&>), int, minimal_hash_registry);

BOOST_OPENMETHOD_OVERRIDE(classify, (const L1_1&), int) { return 1; }
BOOST_OPENMETHOD_OVERRIDE(classify, (const L1_2&), int) { return 2; }
BOOST_OPENMETHOD_OVERRIDE(classify, (const L1_3&), int) { return 3; }
BOOST_OPENMETHOD_OVERRIDE(classify, (const L1_4&), int) { return 4; }
BOOST_OPENMETHOD_OVERRIDE(classify, (const L1_5&), int) { return 5; }
BOOST_OPENMETHOD_OVERRIDE(classify, (const L1_6&), int) { return 6; }
BOOST_OPENMETHOD_OVERRIDE(classify, (const L1_7&), int) { return 7; }
BOOST_OPENMETHOD_OVERRIDE(classify, (const L1_8&), int) { return 8; }
BOOST_OPENMETHOD_OVERRIDE(classify, (const L1_9&), int) { return 9; }
BOOST_OPENMETHOD_OVERRIDE(classify, (const L1_10&), int) { return 10; }

BOOST_AUTO_TEST_CASE(larger_hierarchy) {
    initialize<minimal_hash_registry>();

    L1_1 o1;
    L1_2 o2;
    L1_3 o3;
    L1_4 o4;
    L1_5 o5;
    L1_6 o6;
    L1_7 o7;
    L1_8 o8;
    L1_9 o9;
    L1_10 o10;

    BOOST_TEST(classify(o1) == 1);
    BOOST_TEST(classify(o2) == 2);
    BOOST_TEST(classify(o3) == 3);
    BOOST_TEST(classify(o4) == 4);
    BOOST_TEST(classify(o5) == 5);
    BOOST_TEST(classify(o6) == 6);
    BOOST_TEST(classify(o7) == 7);
    BOOST_TEST(classify(o8) == 8);
    BOOST_TEST(classify(o9) == 9);
    BOOST_TEST(classify(o10) == 10);
}

} // namespace test_large_hierarchy
