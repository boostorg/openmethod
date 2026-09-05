// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// The ADL twin of test_static_rtti.cpp: the same registry, selected by an
// affinity instead of by BOOST_OPENMETHOD_DEFAULT_REGISTRY. Worth its own test
// because a `static_rtti` registry has no `vptr` policy, so every `virtual_ptr`
// has to be created where the exact class is known.

#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/static_rtti.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE adl_registry_static_rtti
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct static_registry : registry<policies::static_rtti> {};

namespace shapes {

struct Shape {
    friend auto boost_openmethod_registry(Shape*) -> static_registry;
};

struct Square : Shape {};
struct Circle : Shape {};

} // namespace shapes

using shapes::Shape, shapes::Square, shapes::Circle;

static_assert(std::is_same_v<registry_affinity<Shape>, static_registry>);
static_assert(
    std::is_same_v<virtual_ptr<Square>, virtual_ptr<Square, static_registry>>);

BOOST_OPENMETHOD_CLASSES(Shape, Square, Circle, static_registry);

BOOST_OPENMETHOD(name, (virtual_ptr<Shape>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (virtual_ptr<Square>), std::string) {
    return "square";
}

BOOST_OPENMETHOD_OVERRIDE(name, (virtual_ptr<Circle>), std::string) {
    return "circle";
}

BOOST_AUTO_TEST_CASE(affinity_works_without_a_vptr_policy) {
    initialize<static_registry>();

    Square square;
    Circle circle;

    BOOST_TEST(name(final_virtual_ptr(square)) == "square");
    BOOST_TEST(name(final_virtual_ptr(circle)) == "circle");
}
