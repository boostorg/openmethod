// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// How a method picks its registry: it takes the affinity its virtual
// parameters agree on. Only a *declared* affinity constrains it, so a class
// with the default affinity yields rather than conflicting. A registry named on
// the declaration wins outright, and no scan happens.

#include <string>

#include <boost/openmethod.hpp>

#define BOOST_TEST_MODULE adl_registry_scan
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

struct zoo_registry : default_registry::with<policies::runtime_checks> {};
struct other_registry : default_registry::with<policies::runtime_checks> {};

namespace zoo {

struct Animal {
    virtual ~Animal() = default;
};

auto boost_openmethod_registry(Animal*) -> zoo_registry;

struct Dog : Animal {};

} // namespace zoo

struct Widget {
    virtual ~Widget() = default;
};

using zoo::Animal, zoo::Dog;

template<typename Fn>
using scan = detail::method_registry<Fn>;

// One virtual parameter with an affinity, in either shape.
static_assert(
    std::is_same_v<scan<void(virtual_<const Animal&>)>, zoo_registry>);
static_assert(std::is_same_v<scan<void(virtual_ptr<Animal>)>, zoo_registry>);
static_assert(std::is_same_v<scan<void(virtual_ptr<Animal>&)>, zoo_registry>);
static_assert(
    std::is_same_v<scan<void(const virtual_ptr<Animal>&)>, zoo_registry>);

// The affinity is inherited, so a derived class carries it too.
static_assert(std::is_same_v<scan<void(virtual_<const Dog&>)>, zoo_registry>);

// No affinity anywhere: the macro default, exactly as before this feature.
static_assert(
    std::is_same_v<scan<void(virtual_<const Widget&>)>, default_registry>);
static_assert(std::is_same_v<scan<void()>, default_registry>);

// Mixing a class that declares an affinity with one that does not: the latter
// yields. This is what keeps a first affinity from cascading errors.
static_assert(std::is_same_v<
              scan<void(virtual_<const Animal&>, virtual_<const Widget&>)>,
              zoo_registry>);
static_assert(std::is_same_v<
              scan<void(virtual_<const Widget&>, virtual_<const Animal&>)>,
              zoo_registry>);

// Mixed shapes agreeing.
static_assert(
    std::is_same_v<
        scan<void(virtual_ptr<Animal>, virtual_<const Dog&>)>, zoo_registry>);

// Non-virtual parameters are ignored.
static_assert(std::is_same_v<
              scan<void(int, virtual_<const Animal&>, char*)>, zoo_registry>);

// A registry spelled on a parameter is what that parameter carries, whatever
// its class declares - so it decides the method's registry on its own.
static_assert(std::is_same_v<
              scan<void(virtual_ptr<Widget, other_registry>)>, other_registry>);
static_assert(
    std::is_same_v<
        scan<void(virtual_<const Widget&, other_registry>)>, other_registry>);
static_assert(std::is_same_v<
              scan<void(virtual_<const Animal&, default_registry>)>,
              default_registry>);

// Two carriers that disagree are an error; see
// compile_fail_method_conflicting_carriers.cpp.

// The whole rule, in one place: every virtual parameter either *carries* a
// registry - the one spelled on it, or the one its class declares - or
// *adopts*, which only a `virtual_` over a class that declares nothing does.
// `virtual_ptr` never adopts: it is a type of its own, and names a registry
// whether or not the class declares an affinity.

// Carriers.
static_assert(
    std::is_same_v<scan<void(virtual_<const Animal&>)>, zoo_registry>);
static_assert(std::is_same_v<scan<void(virtual_ptr<Animal>)>, zoo_registry>);
static_assert(std::is_same_v<
              scan<void(virtual_<const Widget&, zoo_registry>)>, zoo_registry>);
static_assert(std::is_same_v<
              scan<void(virtual_ptr<Widget, zoo_registry>)>, zoo_registry>);
static_assert(
    std::is_same_v<scan<void(virtual_ptr<Widget>)>, default_registry>);

// The only adopter, alone: nothing carries, so the macro default.
static_assert(
    std::is_same_v<scan<void(virtual_<const Widget&>)>, default_registry>);

// An adopter and a carrier: the carrier decides, whichever order.
static_assert(std::is_same_v<
              scan<void(virtual_<const Widget&>, virtual_ptr<Animal>)>,
              zoo_registry>);
static_assert(std::is_same_v<
              scan<void(virtual_ptr<Animal>, virtual_<const Widget&>)>,
              zoo_registry>);
static_assert(std::is_same_v<
              scan<void(virtual_<const Widget&>, virtual_ptr<Widget>)>,
              default_registry>);

// Two carriers that agree, in either shape.
static_assert(
    std::is_same_v<
        scan<void(virtual_<const Animal&>, virtual_ptr<Dog>)>, zoo_registry>);
static_assert(
    std::is_same_v<
        scan<void(virtual_ptr<Widget, zoo_registry>, virtual_<const Animal&>)>,
        zoo_registry>);

// Two adopters: still the macro default.
static_assert(std::is_same_v<
              scan<void(virtual_<const Widget&>, virtual_<const Widget&>)>,
              default_registry>);

// An affinity declared for the default registry itself is declared all the
// same: it constrains, see compile_fail_adl_registry_pinned_default.cpp.
struct Pinned {
    virtual ~Pinned() = default;
    friend auto boost_openmethod_registry(Pinned*) -> default_registry;
};

static_assert(
    std::is_same_v<
        detail::registry_affinity_aux<Pinned>::declared, default_registry>);

// A class that declares nothing carries nothing: `void`, the sentinel that
// makes a `virtual_` parameter adopt the method's registry.
static_assert(
    std::is_same_v<detail::registry_affinity_aux<Widget>::declared, void>);
static_assert(std::is_same_v<
              detail::param_registry<virtual_<const Widget&>>::type, void>);

// A registry named on the declaration wins, and the parameters are not
// consulted at all - the form that predates this feature.
BOOST_OPENMETHOD(ping, (virtual_<const Widget&>), std::string, other_registry);

static_assert(std::is_same_v<
              BOOST_OPENMETHOD_TYPE(
                  ping, (virtual_<const Widget&>), std::string, other_registry),
              method<
                  BOOST_OPENMETHOD_ID(ping),
                  std::string(virtual_<const Widget&>), other_registry>>);

BOOST_AUTO_TEST_CASE(scan_is_compile_time_only) {
    BOOST_TEST(true);
}
