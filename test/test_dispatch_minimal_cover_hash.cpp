// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// First, for BOOST_OPENMETHOD_HAS_PEXT. This header pulls in preamble.hpp but
// not core.hpp, so the default-registry override below is still in time.
#include <boost/openmethod/policies/minimal_cover_hash.hpp>

// `minimal_cover_hash` dispatches with BMI2's `pext`, which not every target
// has; naming the policy in a registry where it is absent is a compile error,
// by design. So the recipe and the cases are conditional, and on a target
// without the instruction this file still builds - as a single case that
// records why it did nothing. The build files add `-mbmi2` (or `/arch:AVX2`) to
// this translation unit alone, on x86 only.
#if BOOST_OPENMETHOD_HAS_PEXT

struct test_registry;
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY test_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>
#include <boost/openmethod/initialize.hpp>

// `runtime_checks` unconditionally, rather than only in a Debug build, so that
// the control table `hash` consults is exercised whatever the build type; and
// `throw_error_handler` so that a lookup of an unregistered class is observable
// from a test case instead of aborting.
struct test_registry :
    boost::openmethod::default_registry::with<
        boost::openmethod::policies::minimal_cover_hash<>,
        boost::openmethod::policies::runtime_checks,
        boost::openmethod::policies::throw_error_handler> {};

#endif

#define BOOST_TEST_MODULE dispatch_minimal_cover_hash
#include <boost/test/unit_test.hpp>

#if BOOST_OPENMETHOD_HAS_PEXT

#include <set>
#include <string>

using namespace boost::openmethod;

namespace {

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};
struct Cat : Animal {};
struct Bulldog : Dog {};
struct Tiger : Cat {};

// Registered nowhere below: calling with one of these must be diagnosed.
struct Ghost : Animal {};

} // namespace

// Withholding `Ghost` is the point of one of the cases, so register explicitly
// and do not call BOOST_OPENMETHOD_REGISTER_CLASSES - see test_classes.hpp.
BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat, Bulldog, Tiger);

BOOST_OPENMETHOD(name, (virtual_<const Animal&>), std::string);
BOOST_OPENMETHOD_OVERRIDE(name, (const Animal&), std::string) {
    return "animal";
}
BOOST_OPENMETHOD_OVERRIDE(name, (const Dog&), std::string) {
    return "dog";
}
BOOST_OPENMETHOD_OVERRIDE(name, (const Cat&), std::string) {
    return "cat";
}
BOOST_OPENMETHOD_OVERRIDE(name, (const Bulldog&), std::string) {
    return "bulldog";
}

BOOST_OPENMETHOD(
    meet, (virtual_<const Animal&>, virtual_<const Animal&>), std::string);
BOOST_OPENMETHOD_OVERRIDE(meet, (const Animal&, const Animal&), std::string) {
    return "ignore";
}
BOOST_OPENMETHOD_OVERRIDE(meet, (const Dog&, const Cat&), std::string) {
    return "chase";
}
BOOST_OPENMETHOD_OVERRIDE(meet, (const Cat&, const Dog&), std::string) {
    return "hiss";
}

using type_hash = test_registry::policy<policies::type_hash>;

BOOST_AUTO_TEST_CASE(single_dispatch) {
    initialize();

    BOOST_TEST(name(Animal()) == "animal");
    BOOST_TEST(name(Dog()) == "dog");
    BOOST_TEST(name(Cat()) == "cat");
    BOOST_TEST(name(Bulldog()) == "bulldog");
    BOOST_TEST(name(Tiger()) == "cat");
}

BOOST_AUTO_TEST_CASE(multiple_dispatch) {
    initialize();

    BOOST_TEST(meet(Dog(), Cat()) == "chase");
    BOOST_TEST(meet(Cat(), Dog()) == "hiss");
    BOOST_TEST(meet(Bulldog(), Tiger()) == "chase");
    BOOST_TEST(meet(Dog(), Dog()) == "ignore");
}

// The property the policy exists for: a cover of bit positions that still
// separates the registered type ids.
BOOST_AUTO_TEST_CASE(hash_is_injective) {
    initialize();

    auto [low, high] = type_hash::hash_range();
    BOOST_TEST(low == 0u);

    std::set<std::size_t> seen;

    for (auto type :
         {&typeid(Animal), &typeid(Dog), &typeid(Cat), &typeid(Bulldog),
          &typeid(Tiger)}) {
        auto index = type_hash::hash(type);
        BOOST_TEST(index >= low);
        BOOST_TEST(index <= high);
        BOOST_TEST(seen.insert(index).second);
    }

    // No minimality bound here: the table is `2^popcount(mask)` and the cover
    // search minimizes the number of *bits*, not the number of slots, so the
    // table can be much larger than the number of type ids. That is the
    // trade this policy makes - see its documentation.
    BOOST_TEST(high + 1 >= seen.size());
}

BOOST_AUTO_TEST_CASE(unregistered_class_is_diagnosed) {
    initialize();

    BOOST_CHECK_THROW(name(Ghost()), missing_class);
}

#else

BOOST_AUTO_TEST_CASE(pext_unavailable) {
    BOOST_TEST_MESSAGE(
        "minimal_cover_hash needs BMI2; BOOST_OPENMETHOD_HAS_PEXT is 0 on this "
        "target, so there is nothing to test here");
    BOOST_TEST(BOOST_OPENMETHOD_HAS_PEXT == 0);
}

#endif
