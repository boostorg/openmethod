// Copyright (c) 2018-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <any>

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/std_any.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

#define MAKE_CLASSES()                                                         \
    struct Dog {                                                               \
        std::string name;                                                      \
    };                                                                         \
                                                                               \
    use_any_types<Dog, std::string, int> BOOST_OPENMETHOD_GENSYM;

#if 0

namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as std::any by value

MAKE_CLASSES();

BOOST_OPENMETHOD(name, (virtual_<std::any>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (Dog dog), std::string) {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(name, (Cat cat), std::string) {
    return cat.name + " the cat";
}

BOOST_AUTO_TEST_CASE(std_any_by_value) {
    initialize();

    BOOST_TEST(name(std::any(Dog{"Spot"})) == "Spot the dog");
    BOOST_TEST(name(std::any(Cat{"Felix"})) == "Felix the cat");
}
} // namespace BOOST_OPENMETHOD_GENSYM
#endif
namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as const std::any& (const ref)

static_assert(detail::has_dynamic_vptr<
              virtual_traits<const std::any&, default_registry>, type_id>);

MAKE_CLASSES();

BOOST_OPENMETHOD(name, (virtual_<const std::any&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (const Dog& dog), std::string) {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(name, (const std::string& name), std::string) {
    return name;
}

BOOST_OPENMETHOD_OVERRIDE(name, (const int& value), std::string) {
    std::ostringstream os;
    os << value << " the integer";
    return os.str();
}

BOOST_AUTO_TEST_CASE(std_any_by_const_ref) {
    initialize(trace());

    const std::any spot(Dog{"Spot"});
    const std::any felix(std::string{"Felix the cat"});
    const std::any answer(42);

    BOOST_TEST(name(spot) == "Spot the dog");
    BOOST_TEST(name(felix) == "Felix the cat");
    BOOST_TEST(name(answer) == "42 the integer");
}
} // namespace BOOST_OPENMETHOD_GENSYM
#if 0
namespace BOOST_OPENMETHOD_GENSYM {

// -----------------------------------------------------------------------------
// pass virtual args as std::any&& (rvalue ref, move semantics)

MAKE_CLASSES();

BOOST_OPENMETHOD(name, (virtual_<std::any&&>), std::string);

BOOST_OPENMETHOD_OVERRIDE(name, (Dog dog), std::string) {
    return dog.name + " the dog";
}

BOOST_OPENMETHOD_OVERRIDE(name, (Cat cat), std::string) {
    return cat.name + " the cat";
}

BOOST_AUTO_TEST_CASE(std_any_by_rvalue_ref) {
    initialize();

    std::any spot(Dog{"Spot"});
    std::any felix(Cat{"Felix"});

    BOOST_TEST(name(std::move(spot)) == "Spot the dog");
    BOOST_TEST(name(std::move(felix)) == "Felix the cat");
}
} // namespace BOOST_OPENMETHOD_GENSYM
#endif