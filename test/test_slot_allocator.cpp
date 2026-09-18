// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <boost/mp11/algorithm.hpp>
#include <boost/mp11/list.hpp>
#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/policies/vptr_map.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE slot_allocator
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

using class_ = detail::generic_compiler::class_;
using cc_method = detail::generic_compiler::method;
using overrider = detail::generic_compiler::overrider;

auto operator<<(std::ostream& os, const class_* cls) -> std::ostream& {
    return os
        << reinterpret_cast<const std::type_info*>(cls->ci[0]->type)->name();
}

std::string empty = "{}";

template<template<typename...> class Container, typename T>
auto str(const Container<T>& container) {
    std::ostringstream os;
    os << "{";
    const char* sep = "";

    for (const auto& item : container) {
        os << sep << item;
        sep = ", ";
    }

    os << "}";

    return os.str();
}

template<typename... Ts>
auto sstr(Ts... args) {
    std::vector<class_*> vec{args...};
    std::sort(vec.begin(), vec.end());

    return str(vec);
}

template<typename T>
auto sstr(const std::unordered_set<T>& container) {
    return sstr(std::vector<T>(container.begin(), container.end()));
}

template<typename T, typename Compiler>
auto get_class(const Compiler& comp) {
    // Key through the rtti policy rather than with a raw typeid: the policy
    // decides what identifies a class (std_rtti uses the mangled name, see
    // std_rtti::type_index), and only it knows how to derive that from a type.
    return comp.class_map.at(
        boost::openmethod::default_registry::rtti::type_index(&typeid(T)));
}

/*
A   B
 \ / \
 AB   D
 |    |
 C    E
  \  /
   F
*/

namespace diamond {

struct A {
    virtual ~A() {
    }
};

struct B {
    virtual ~B() {
    }
};

struct AB : A, B {};

struct C : AB {};

struct D : B {};

struct E : D {};

struct F : C, E {};

} // namespace diamond

// ============================================================================
// Test use_classes.

BOOST_AUTO_TEST_CASE(test_use_classes_linear) {
    struct Base {
        virtual ~Base() = default;
    };

    struct D1 : Base {};
    struct D2 : D1 {};
    struct D3 : D2 {};
    struct D4 : D3 {};
    struct D5 : D4 {};

    struct registry : test_registry_<__COUNTER__> {};

    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_CLASSES expands through
    // BOOST_OPENMETHOD_REGISTER, now `inline` - illegal at block scope - so
    // it's bypassed here in favor of its own expansion, spelled out by hand.
    static use_classes<Base, D1, D2, D3, registry> BOOST_OPENMETHOD_GENSYM;
    static use_classes<D2, D3, registry> BOOST_OPENMETHOD_GENSYM;
    static use_classes<D3, D4, registry> BOOST_OPENMETHOD_GENSYM;
    static use_classes<D4, D5, D3, registry> BOOST_OPENMETHOD_GENSYM;

    auto comp = initialize<registry>();

    auto base = get_class<Base>(comp);
    auto d1 = get_class<D1>(comp);
    auto d2 = get_class<D2>(comp);
    auto d3 = get_class<D3>(comp);
    auto d4 = get_class<D4>(comp);
    auto d5 = get_class<D5>(comp);

    BOOST_CHECK_EQUAL(sstr(base->direct_bases), empty);
    BOOST_CHECK_EQUAL(sstr(base->direct_derived), sstr(d1));
    BOOST_CHECK_EQUAL(
        sstr(base->transitive_derived), sstr(base, d1, d2, d3, d4, d5));

    BOOST_CHECK_EQUAL(sstr(d1->direct_derived), sstr(d2));
    BOOST_CHECK_EQUAL(sstr(d1->direct_bases), sstr(base));
    BOOST_CHECK_EQUAL(sstr(d1->transitive_derived), sstr(d1, d2, d3, d4, d5));

    BOOST_CHECK_EQUAL(sstr(d2->direct_derived), sstr(d3));
    BOOST_CHECK_EQUAL(sstr(d2->direct_bases), sstr(d1));
    BOOST_CHECK_EQUAL(sstr(d2->transitive_derived), sstr(d2, d3, d4, d5));

    BOOST_CHECK_EQUAL(sstr(d3->direct_derived), sstr(d4));
    BOOST_CHECK_EQUAL(sstr(d3->direct_bases), sstr(d2));
    BOOST_CHECK_EQUAL(sstr(d3->transitive_derived), sstr(d3, d4, d5));

    BOOST_CHECK_EQUAL(sstr(d4->direct_derived), sstr(d5));
    BOOST_CHECK_EQUAL(sstr(d4->direct_bases), sstr(d3));
    BOOST_CHECK_EQUAL(sstr(d4->transitive_derived), sstr(d4, d5));
}

// The lattice must not depend on the order in which classes are registered.
// Every record below carries exactly one, genuinely direct, base; only the order
// of the calls differs from `test_use_classes_linear`, with D3's own ancestry
// registered last.
BOOST_AUTO_TEST_CASE(test_use_classes_derived_before_base) {
    struct Base {
        virtual ~Base() = default;
    };

    struct D1 : Base {};
    struct D2 : D1 {};
    struct D3 : D2 {};
    struct D4 : D3 {};
    struct D5 : D4 {};

    struct registry : test_registry_<__COUNTER__> {};

    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_CLASSES expands through
    // BOOST_OPENMETHOD_REGISTER, now `inline` - illegal at block scope - so
    // it's bypassed here in favor of its own expansion, spelled out by hand.
    static use_classes<D3, D4, registry> BOOST_OPENMETHOD_GENSYM;
    static use_classes<D4, D5, registry> BOOST_OPENMETHOD_GENSYM;
    static use_classes<Base, D1, D2, D3, registry> BOOST_OPENMETHOD_GENSYM;

    auto comp = initialize<registry>();

    auto base = get_class<Base>(comp);
    auto d1 = get_class<D1>(comp);
    auto d2 = get_class<D2>(comp);
    auto d3 = get_class<D3>(comp);
    auto d4 = get_class<D4>(comp);
    auto d5 = get_class<D5>(comp);

    BOOST_CHECK_EQUAL(sstr(base->direct_bases), empty);
    BOOST_CHECK_EQUAL(sstr(d1->direct_bases), sstr(base));
    BOOST_CHECK_EQUAL(sstr(d2->direct_bases), sstr(d1));
    BOOST_CHECK_EQUAL(sstr(d3->direct_bases), sstr(d2));
    BOOST_CHECK_EQUAL(sstr(d4->direct_bases), sstr(d3));
    // D3 is an *indirect* base of D5, and must not appear here.
    BOOST_CHECK_EQUAL(sstr(d5->direct_bases), sstr(d4));

    BOOST_CHECK_EQUAL(sstr(d5->transitive_bases), sstr(base, d1, d2, d3, d4));
    BOOST_CHECK_EQUAL(sstr(d4->transitive_bases), sstr(base, d1, d2, d3));

    BOOST_CHECK_EQUAL(sstr(base->direct_derived), sstr(d1));
    BOOST_CHECK_EQUAL(sstr(d3->direct_derived), sstr(d4));
    BOOST_CHECK_EQUAL(sstr(d4->direct_derived), sstr(d5));
    BOOST_CHECK_EQUAL(
        sstr(base->transitive_derived), sstr(base, d1, d2, d3, d4, d5));
}

BOOST_AUTO_TEST_CASE(test_use_classes_diamond) {
    using test_registry = test_registry_<__COUNTER__>;
    using namespace diamond;
    // Function-local static: registers on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<A, B, AB, C, D, E, test_registry>
        BOOST_OPENMETHOD_GENSYM;

    std::vector<class_*> actual, expected;

    auto comp = initialize<test_registry>();

    auto a = get_class<A>(comp);
    auto b = get_class<B>(comp);
    auto ab = get_class<AB>(comp);
    auto c = get_class<C>(comp);
    auto d = get_class<D>(comp);
    auto e = get_class<E>(comp);

    // -----------------------------------------------------------------------
    // A
    BOOST_REQUIRE_EQUAL(sstr(a->direct_bases), empty);
    BOOST_REQUIRE_EQUAL(sstr(a->direct_derived), sstr(ab));
    BOOST_REQUIRE_EQUAL(sstr(a->transitive_derived), sstr(a, ab, c));

    // -----------------------------------------------------------------------
    // B
    BOOST_REQUIRE_EQUAL(sstr(b->direct_bases), empty);
    BOOST_REQUIRE_EQUAL(sstr(b->direct_derived), sstr(ab, d));
    BOOST_REQUIRE_EQUAL(sstr(b->transitive_derived), sstr(b, ab, c, d, e));

    // -----------------------------------------------------------------------
    // AB
    BOOST_REQUIRE_EQUAL(sstr(ab->direct_bases), sstr(a, b));
    BOOST_REQUIRE_EQUAL(sstr(ab->direct_derived), sstr(c));
    BOOST_REQUIRE_EQUAL(sstr(ab->transitive_derived), sstr(ab, c));

    // -----------------------------------------------------------------------
    // C
    BOOST_REQUIRE_EQUAL(sstr(c->direct_bases), sstr(ab));
    BOOST_REQUIRE_EQUAL(sstr(c->direct_derived), empty);
    BOOST_REQUIRE_EQUAL(sstr(c->transitive_derived), sstr(c));

    // -----------------------------------------------------------------------
    // D
    BOOST_REQUIRE_EQUAL(sstr(d->direct_bases), sstr(b));
    BOOST_REQUIRE_EQUAL(sstr(d->direct_derived), sstr(e));
    BOOST_REQUIRE_EQUAL(sstr(d->transitive_derived), sstr(d, e));

    // -----------------------------------------------------------------------
    // E
    BOOST_REQUIRE_EQUAL(sstr(e->direct_bases), sstr(d));
    BOOST_REQUIRE_EQUAL(sstr(e->direct_derived), empty);
    BOOST_REQUIRE_EQUAL(sstr(e->transitive_derived), sstr(e));
}

/// ============================================================================
// Test slot allocation.
//
// The order in which classes are registered is not under a program's control:
// it is the order of static construction across translation units, and across
// modules. The allocator must be correct in every order, and the tests must
// not assume one. As the compiler sees it, that order is the order of the
// class records in the registry's class list, so the harness rewrites it:
// it unlinks the records of the hierarchy under test and links them back in
// the order it wants - every permutation for small hierarchies, a fixed
// sample for larger ones. What is checked is what holds in every order: no
// two parameters share a slot in any v-table, every v-table is as tight as
// its slots allow, and the total size of the hierarchy's v-tables is what the
// expectations say. For every hierarchy below, the harness found the total to
// be the same in every order; the comments give what the old allocator,
// before issue #19, produced over the same orders.
//
// Every hierarchy is a namespace, registered once, into one of two registries:
// one for the small lattices, one for the real hierarchies. Hierarchies
// sharing a registry are separate components of the class graph, and the
// allocator handles components independently, so each test sees the others
// only through the size of the registry it initializes. Compiling a registry's
// `initialize` is expensive, and so is every `use_classes`: the tests share
// both as much as they can.

auto check(const detail::generic_compiler::method* method)
    -> const detail::generic_compiler::method* {
    if (method) {
        return method;
    }

    BOOST_FAIL("method not found");

    return nullptr;
}

template<int>
struct M;

// A unary method dispatching on CLASS, bound to a local reference.
#define ADD_METHOD(CLASS)                                                      \
    auto& BOOST_PP_CAT(m_, CLASS) =                                            \
        method<CLASS, auto(virtual_<CLASS&>)->void, test_registry>::fn;

#define ADD_METHOD_N(CLASS, N)                                                 \
    auto& BOOST_PP_CAT(BOOST_PP_CAT(m_, CLASS), N) =                           \
        method<M<N>, auto(virtual_<CLASS&>)->void, test_registry>::fn;

// A unary method dispatching on CLASS, with no name to refer to it by.
#define USE_METHOD(CLASS)                                                      \
    (void)&method<CLASS, auto(virtual_<CLASS&>)->void, test_registry>::fn

#define USE_METHOD_N(CLASS, N)                                                 \
    (void)&method<M<N>, auto(virtual_<CLASS&>)->void, test_registry>::fn

// A binary method dispatching on CLASS1 and CLASS2.
#define USE_METHOD2(CLASS1, CLASS2)                                            \
    (void)&method<                                                             \
        M<__LINE__>, auto(virtual_<CLASS1&>, virtual_<CLASS2&>)->void,         \
        test_registry>::fn

// `initialize` spends most of its time in fast_perfect_hash, searching for a
// perfect hash of the registry's type ids, and the search grows steeply with
// their number. The slot allocator does not need it: the shared registries
// use the map-based v-table pointer policy and drop the hash, so that the
// thousands of initializations the tests run stay cheap.
struct lattices_registry :
    default_registry::with<unique<__COUNTER__>, policies::vptr_map<>>::without<
        policies::fast_perfect_hash> {};
struct hierarchies_registry :
    default_registry::with<unique<__COUNTER__>, policies::vptr_map<>>::without<
        policies::fast_perfect_hash> {};

using order = std::vector<std::size_t>;

auto all_orders(std::size_t n) -> std::vector<order> {
    order permutation(n);
    std::iota(permutation.begin(), permutation.end(), std::size_t(0));
    std::vector<order> orders;

    do {
        orders.push_back(permutation);
    } while (std::next_permutation(permutation.begin(), permutation.end()));

    return orders;
}

// The listed order, its reverse, and `samples` shuffles. The shuffle is
// spelled out, rather than left to std::shuffle, so that every platform sees
// the same orders.
auto sampled_orders(std::size_t n, std::size_t samples) -> std::vector<order> {
    order listed(n);
    std::iota(listed.begin(), listed.end(), std::size_t(0));
    std::vector<order> orders{listed};
    orders.emplace_back(listed.rbegin(), listed.rend());
    std::mt19937 rng(20250510u);
    auto shuffled = listed;

    while (samples--) {
        for (auto i = shuffled.size(); i > 1; --i) {
            std::swap(shuffled[i - 1], shuffled[rng() % i]);
        }

        orders.push_back(shuffled);
    }

    return orders;
}

// The registry's record of each class, in the order of the list.
template<class Registry, class... Classes>
auto records_of(boost::mp11::mp_list<Classes...>)
    -> std::vector<detail::class_info*> {
    std::vector<detail::class_info*> records;

    auto find = [&records](type_id type) {
        for (auto& record : Registry::state().classes) {
            if (Registry::rtti::type_index(record.type) ==
                Registry::rtti::type_index(type)) {
                records.push_back(&record);
                return;
            }
        }

        BOOST_FAIL("class not registered");
    };

    (find(Registry::rtti::template static_type<Classes>()), ...);

    return records;
}

// Move the records to the end of the registry's class list, in `by`.
template<class Registry>
void reorder(const std::vector<detail::class_info*>& records, const order& by) {
    auto& list = Registry::state().classes;

    for (auto record : records) {
        list.remove(*record);
    }

    for (auto i : by) {
        list.push_back(*records[i]);
    }
}

// Every v-table holds the slots of every class whose methods can be called
// on the class - the classes it is in the cone of - each at most once, and
// starts at the first of them and ends at the last. One assertion per run:
// the harness runs this thousands of times, and Boost.Test assertions are not
// cheap.
template<class Compiler>
void check_slots(const Compiler& comp) {
    std::unordered_map<const class_*, std::vector<std::size_t>> held;

    for (const auto& cls : comp.classes) {
        for (const auto& mp : cls.used_by_vp) {
            for (auto derived : cls.transitive_derived) {
                held[derived].push_back(mp.method->slots[mp.param]);
            }
        }
    }

    std::ostringstream problems;

    for (const auto& cls : comp.classes) {
        auto& slots = held[&cls];
        std::sort(slots.begin(), slots.end());

        if (std::adjacent_find(slots.begin(), slots.end()) != slots.end()) {
            problems << &cls << ": a slot is held twice\n";
        }

        if (slots.empty()) {
            if (!cls.vtbl.empty() || cls.first_slot != 0) {
                problems << &cls << ": a v-table without slots\n";
            }
        } else if (
            cls.first_slot != slots.front() ||
            cls.first_slot + cls.vtbl.size() != slots.back() + 1) {
            problems << &cls << ": v-table " << cls.first_slot << "-"
                     << (cls.first_slot + cls.vtbl.size()) << " for slots "
                     << slots.front() << "-" << slots.back() << "\n";
        }
    }

    BOOST_TEST(problems.str().empty(), problems.str());
}

// The total size of the v-tables of the listed classes.
template<class Compiler, class... Classes>
auto total_vtbl_size(const Compiler& comp, boost::mp11::mp_list<Classes...>)
    -> std::size_t {
    return (std::size_t(0) + ... + get_class<Classes>(comp)->vtbl.size());
}

struct allocation_stats {
    std::size_t runs = 0;
    std::size_t min_total = std::numeric_limits<std::size_t>::max();
    std::size_t max_total = 0;
};

// Register `Classes` in each of `orders`, initialize, check the invariants,
// let `fn` check what the test expects, and collect the total v-table size.
template<class Registry, class Classes, class Fn>
auto allocate_in_orders(const std::vector<order>& orders, Fn fn)
    -> allocation_stats {
    auto records = records_of<Registry>(Classes{});
    allocation_stats stats;

    for (const auto& registration_order : orders) {
        BOOST_TEST_CONTEXT("registration order " << str(registration_order)) {
            reorder<Registry>(records, registration_order);
            auto comp = initialize<Registry>();
            check_slots(comp);
            auto total = total_vtbl_size(comp, Classes{});
            stats.min_total = std::min(stats.min_total, total);
            stats.max_total = std::max(stats.max_total, total);
            ++stats.runs;
            fn(comp);
        }
    }

    finalize<Registry>();

    BOOST_TEST_MESSAGE(
        boost::unit_test::framework::current_test_case().p_name
        << ": " << stats.runs << " orders, total v-table size "
        << stats.min_total << "-" << stats.max_total);

    return stats;
}

void expect_total(const allocation_stats& stats, std::size_t total) {
    BOOST_TEST(stats.min_total == total);
    BOOST_TEST(stats.max_total == total);
}

// ----------------------------------------------------------------------------
// The slot chooser on its own.

BOOST_AUTO_TEST_CASE(test_pick_slot) {
    using boost::dynamic_bitset;
    using detail::generic_compiler;

    auto bits = [](std::initializer_list<std::size_t> slots) {
        dynamic_bitset<> result;

        for (auto slot : slots) {
            detail::set_bit(result, slot);
        }

        return result;
    };

    auto pick = [&](std::vector<dynamic_bitset<>> used) {
        std::vector<class_> classes(used.size());
        std::vector<class_*> cone;
        dynamic_bitset<> unavailable;

        for (std::size_t i = 0; i < used.size(); ++i) {
            classes[i].used_slots = used[i];
            cone.push_back(&classes[i]);
            detail::merge_into(used[i], unavailable);
        }

        return generic_compiler::pick_slot(cone, unavailable);
    };

    // Nothing allocated anywhere: slot 0, one entry per class.
    auto choice = pick({bits({}), bits({})});
    BOOST_TEST(choice.slot == 0u);
    BOOST_TEST(choice.cost == 2u);

    // A single range grows upward.
    choice = pick({bits({0, 1, 2})});
    BOOST_TEST(choice.slot == 3u);
    BOOST_TEST(choice.cost == 1u);

    // A hole in the range costs nothing.
    choice = pick({bits({0, 2})});
    BOOST_TEST(choice.slot == 1u);
    BOOST_TEST(choice.cost == 0u);

    // Up and down cost the same: the lowest slot wins.
    choice = pick({bits({1})});
    BOOST_TEST(choice.slot == 0u);
    BOOST_TEST(choice.cost == 1u);

    // Down is blocked by a derived class: up.
    choice = pick({bits({2}), bits({0, 1, 2})});
    BOOST_TEST(choice.slot == 3u);
    BOOST_TEST(choice.cost == 2u);

    // The derived classes' ranges count: down fills a hole in the derived
    // class, up grows it.
    choice = pick({bits({5}), bits({0, 1, 2, 3, 5})});
    BOOST_TEST(choice.slot == 4u);
    BOOST_TEST(choice.cost == 1u);

    // A class without a range lands where its derived classes grow least.
    choice = pick({bits({}), bits({0, 1}), bits({0, 1})});
    BOOST_TEST(choice.slot == 2u);
    BOOST_TEST(choice.cost == 3u);

    // ...and inside a hole they share, if there is one.
    choice = pick({bits({}), bits({0, 2}), bits({0, 2, 3})});
    BOOST_TEST(choice.slot == 1u);
    BOOST_TEST(choice.cost == 1u);
}

// ----------------------------------------------------------------------------
// Small lattices, in every registration order.

namespace a_b1_c {

/*
  A
 / \
B1  C
*/

struct A {
    virtual ~A() = default;
};
struct B : A {};
struct C : A {};

using classes = boost::mp11::mp_list<A, B, C>;
BOOST_OPENMETHOD_REGISTER(use_classes<A, B, C, lattices_registry>);

} // namespace a_b1_c

BOOST_AUTO_TEST_CASE(test_assign_slots_a_b1_c) {
    using test_registry = lattices_registry;
    using namespace a_b1_c;

    ADD_METHOD(B);

    auto stats = allocate_in_orders<test_registry, classes>(
        all_orders(3), [&](const auto& comp) {
            BOOST_TEST_REQUIRE(check(comp[m_B])->slots.size() == 1u);
            BOOST_TEST(check(comp[m_B])->slots[0] == 0u);
            BOOST_TEST(get_class<A>(comp)->vtbl.size() == 0u);
            BOOST_TEST(get_class<B>(comp)->vtbl.size() == 1u);
            BOOST_TEST(get_class<C>(comp)->vtbl.size() == 0u);
        });

    expect_total(stats, 1);
}

namespace a1_b1_c1 {

/*
  A1
 / \
B1  C1
*/

struct A {
    virtual ~A() = default;
};
struct B : A {};
struct C : A {};

using classes = boost::mp11::mp_list<A, B, C>;
BOOST_OPENMETHOD_REGISTER(use_classes<A, B, C, lattices_registry>);

} // namespace a1_b1_c1

BOOST_AUTO_TEST_CASE(test_assign_slots_a1_b1_c1) {
    using test_registry = lattices_registry;
    using namespace a1_b1_c1;

    ADD_METHOD(A);
    ADD_METHOD(B);
    ADD_METHOD(C);

    auto stats = allocate_in_orders<test_registry, classes>(
        all_orders(3), [&](const auto& comp) {
            BOOST_TEST(check(comp[m_A])->slots[0] == 0u);
            BOOST_TEST(check(comp[m_B])->slots[0] == 1u);
            BOOST_TEST(check(comp[m_C])->slots[0] == 1u);
            BOOST_TEST(get_class<A>(comp)->vtbl.size() == 1u);
            BOOST_TEST(get_class<B>(comp)->vtbl.size() == 2u);
            BOOST_TEST(get_class<C>(comp)->vtbl.size() == 2u);
        });

    expect_total(stats, 5);
}

namespace tree_any_order {

/*
  A1
 / \
B1  C1
|
D1
*/

struct A {
    virtual ~A() = default;
};
struct B : A {};
struct C : A {};
struct D : B {};

using classes = boost::mp11::mp_list<A, B, C, D>;
BOOST_OPENMETHOD_REGISTER(use_classes<A, B, C, D, lattices_registry>);

} // namespace tree_any_order

BOOST_AUTO_TEST_CASE(test_assign_slots_tree_any_order) {
    using test_registry = lattices_registry;
    using namespace tree_any_order;

    ADD_METHOD(A);
    ADD_METHOD(B);
    ADD_METHOD(C);
    ADD_METHOD(D);

    // In a tree, every v-table is dense from slot 0, whatever the order.
    auto stats = allocate_in_orders<test_registry, classes>(
        all_orders(4), [&](const auto& comp) {
            BOOST_TEST(check(comp[m_A])->slots[0] == 0u);
            BOOST_TEST(check(comp[m_B])->slots[0] == 1u);
            BOOST_TEST(check(comp[m_C])->slots[0] == 1u);
            BOOST_TEST(check(comp[m_D])->slots[0] == 2u);
            BOOST_TEST(get_class<A>(comp)->first_slot == 0u);
            BOOST_TEST(get_class<B>(comp)->first_slot == 0u);
            BOOST_TEST(get_class<C>(comp)->first_slot == 0u);
            BOOST_TEST(get_class<D>(comp)->first_slot == 0u);
            BOOST_TEST(get_class<A>(comp)->vtbl.size() == 1u);
            BOOST_TEST(get_class<B>(comp)->vtbl.size() == 2u);
            BOOST_TEST(get_class<C>(comp)->vtbl.size() == 2u);
            BOOST_TEST(get_class<D>(comp)->vtbl.size() == 3u);
        });

    expect_total(stats, 8);
}

namespace a1_b1_d1_c1_d1 {

/*
  A1
 / \
B1  C1
 \ /
  D1
*/

struct A {
    virtual ~A() = default;
};
struct B : virtual A {};
struct C : virtual A {};
struct D : B, C {};

using classes = boost::mp11::mp_list<A, B, C, D>;
BOOST_OPENMETHOD_REGISTER(use_classes<A, B, C, D, lattices_registry>);

} // namespace a1_b1_d1_c1_d1

BOOST_AUTO_TEST_CASE(test_assign_slots_a1_b1_d1_c1_d1) {
    using test_registry = lattices_registry;
    using namespace a1_b1_d1_c1_d1;

    ADD_METHOD(A);
    USE_METHOD(B);
    USE_METHOD(C);
    ADD_METHOD(D);

    // B and C both sit above A's slot and must differ in D, so one of them has
    // a hole - there is no way around that. The old allocator gave the other
    // one a hole too, for a total of 11.
    auto stats = allocate_in_orders<test_registry, classes>(
        all_orders(4), [&](const auto& comp) {
            BOOST_TEST(check(comp[m_A])->slots[0] == 0u);
            BOOST_TEST(check(comp[m_D])->slots[0] == 3u);
            BOOST_TEST(get_class<A>(comp)->vtbl.size() == 1u);
            BOOST_TEST(get_class<D>(comp)->vtbl.size() == 4u);
            auto b = get_class<B>(comp)->vtbl.size();
            auto c = get_class<C>(comp)->vtbl.size();
            BOOST_TEST(std::min(b, c) == 2u);
            BOOST_TEST(std::max(b, c) == 3u);
        });

    expect_total(stats, 10);
}

namespace a1_b1_d1_c1_d1_e3 {

/*
  A1
 / \
B1  C1
 \ /  \
  D1  E3
*/

struct A {
    virtual ~A() = default;
};
struct B : virtual A {};
struct C : virtual A {};
struct E : C {};
struct D : B, C {};

using classes = boost::mp11::mp_list<A, B, C, E, D>;
BOOST_OPENMETHOD_REGISTER(use_classes<A, B, C, E, D, lattices_registry>);

} // namespace a1_b1_d1_c1_d1_e3

BOOST_AUTO_TEST_CASE(test_assign_slots_a1_b1_d1_c1_d1_e3) {
    using test_registry = lattices_registry;
    using namespace a1_b1_d1_c1_d1_e3;

    ADD_METHOD(A);
    USE_METHOD(B);
    USE_METHOD(C);
    ADD_METHOD(D);
    USE_METHOD_N(E, 1);
    USE_METHOD_N(E, 2);
    USE_METHOD_N(E, 3);

    // E fills the hole in C's v-table, if C has one. Old total: 16.
    auto stats = allocate_in_orders<test_registry, classes>(
        all_orders(5), [&](const auto& comp) {
            BOOST_TEST(check(comp[m_A])->slots[0] == 0u);
            BOOST_TEST(check(comp[m_D])->slots[0] == 3u);
            BOOST_TEST(get_class<A>(comp)->vtbl.size() == 1u);
            BOOST_TEST(get_class<D>(comp)->vtbl.size() == 4u);
            BOOST_TEST(get_class<E>(comp)->vtbl.size() == 5u);
        });

    expect_total(stats, 15);
}

namespace a1_c1_b1 {

/*
A1  B1
 \  /
  C1
*/

struct A {
    virtual ~A() = default;
};
struct B {
    virtual ~B() = default;
};
struct C : A, B {};

using classes = boost::mp11::mp_list<A, B, C>;
BOOST_OPENMETHOD_REGISTER(use_classes<A, B, C, lattices_registry>);

} // namespace a1_c1_b1

BOOST_AUTO_TEST_CASE(test_assign_slots_a1_c1_b1) {
    using test_registry = lattices_registry;
    using namespace a1_c1_b1;

    ADD_METHOD(A);
    ADD_METHOD(B);
    ADD_METHOD(C);

    // The second root to be allocated gets slot 1; its v-table starts there,
    // since leading unused slots are not stored.
    auto stats = allocate_in_orders<test_registry, classes>(
        all_orders(3), [&](const auto& comp) {
            auto a = get_class<A>(comp);
            auto b = get_class<B>(comp);
            BOOST_TEST(
                check(comp[m_A])->slots[0] + check(comp[m_B])->slots[0] == 1u);
            BOOST_TEST(check(comp[m_C])->slots[0] == 2u);
            BOOST_TEST(a->vtbl.size() == 1u);
            BOOST_TEST(b->vtbl.size() == 1u);
            BOOST_TEST(a->first_slot + b->first_slot == 1u);
            BOOST_TEST(get_class<C>(comp)->vtbl.size() == 3u);
        });

    expect_total(stats, 5);
}

namespace hole_avoided {

/*
A1  B1
 \ / \
 C1   E1
*/

struct A {
    virtual ~A() = default;
};
struct B {
    virtual ~B() = default;
};
struct C : A, B {};
struct E : B {};

using classes = boost::mp11::mp_list<A, B, C, E>;
BOOST_OPENMETHOD_REGISTER(use_classes<A, B, C, E, lattices_registry>);

} // namespace hole_avoided

BOOST_AUTO_TEST_CASE(test_assign_slots_hole_avoided) {
    using test_registry = lattices_registry;
    using namespace hole_avoided;

    USE_METHOD(A);
    USE_METHOD(B);
    USE_METHOD(C);
    USE_METHOD(E);

    // E extends B's range by one entry. The old allocator, when it visited A,
    // C, B, E, put B at 2 and E at the lowest free slot, 0: a three-entry
    // v-table for E with a hole at 1, and a total of 8 in half of the orders.
    auto stats = allocate_in_orders<test_registry, classes>(
        all_orders(4), [&](const auto& comp) {
            BOOST_TEST(get_class<C>(comp)->vtbl.size() == 3u);
            BOOST_TEST(get_class<E>(comp)->vtbl.size() == 2u);
        });

    expect_total(stats, 7);
}

namespace grow_down {

/*
A2      B1     W1
|\ \   / \    / |
Aa Ab C   E1  |
         \   |
          G--'     and V : A, W
*/

struct A {
    virtual ~A() = default;
};
struct Aa : A {};
struct Ab : A {};
struct B {
    virtual ~B() = default;
};
struct W {
    virtual ~W() = default;
};
struct C : A, B {};
struct V : A, W {};
struct E : B {};
struct G : E, W {};

using classes = boost::mp11::mp_list<A, Aa, Ab, B, W, C, V, E, G>;
BOOST_OPENMETHOD_REGISTER(
    use_classes<A, Aa, Ab, B, W, C, V, E, G, lattices_registry>);

} // namespace grow_down

BOOST_AUTO_TEST_CASE(test_assign_slots_grow_down) {
    using test_registry = lattices_registry;
    using namespace grow_down;

    USE_METHOD_N(A, 1);
    USE_METHOD_N(A, 2);
    ADD_METHOD(B);
    ADD_METHOD(W);
    ADD_METHOD(E);

    // The cones decide the order: A (five classes) takes 0 and 1, B (four)
    // 2, W (three) 3 - G already holds 2 - and E, inheriting 2 from B, can
    // only extend downward, since G holds 3. The old allocator gave E the
    // lowest free slot, 0, with a hole at 1, in most orders: 20 to 23 in
    // total, against 20 in every order here.
    auto stats = allocate_in_orders<test_registry, classes>(
        sampled_orders(9, 200), [&](const auto& comp) {
            BOOST_TEST(check(comp[m_B])->slots[0] == 2u);
            BOOST_TEST(check(comp[m_W])->slots[0] == 3u);
            BOOST_TEST(check(comp[m_E])->slots[0] == 1u);
            BOOST_TEST(get_class<E>(comp)->first_slot == 1u);
            BOOST_TEST(get_class<E>(comp)->vtbl.size() == 2u);
        });

    expect_total(stats, 20);
}

namespace cone_aware {

/*
Y5   P1   R4        Ya, Yb, Yc : Y
 \  / \  / \        Ra, Rb, Rc : R
  Z    X1   \
       \    |
        D---'
*/

struct Y {
    virtual ~Y() = default;
};
struct Ya : Y {};
struct Yb : Y {};
struct Yc : Y {};
struct P {
    virtual ~P() = default;
};
struct R {
    virtual ~R() = default;
};
struct Ra : R {};
struct Rb : R {};
struct Rc : R {};
struct Z : Y, P {};
struct X : P {};
struct D : X, R {};

using classes = boost::mp11::mp_list<Y, Ya, Yb, Yc, P, R, Ra, Rb, Rc, Z, X, D>;
BOOST_OPENMETHOD_REGISTER(
    use_classes<Y, Ya, Yb, Yc, P, R, Ra, Rb, Rc, Z, X, D, lattices_registry>);

} // namespace cone_aware

BOOST_AUTO_TEST_CASE(test_assign_slots_cone_aware) {
    using test_registry = lattices_registry;
    using namespace cone_aware;

    USE_METHOD_N(Y, 1);
    USE_METHOD_N(Y, 2);
    USE_METHOD_N(Y, 3);
    USE_METHOD_N(Y, 4);
    USE_METHOD_N(Y, 5);
    ADD_METHOD(P);
    USE_METHOD_N(R, 1);
    USE_METHOD_N(R, 2);
    USE_METHOD_N(R, 3);
    USE_METHOD_N(R, 4);
    ADD_METHOD(X);

    // Y and R, with the widest cones, take 0-4 and 0-3; P lands at 5, and X
    // inherits {5}. X's own range would grow by one either way, but D, which
    // holds 0-3 and 5, has a hole at 4 and would grow at 6: X goes down. The
    // old allocator: 51 to 55, depending on the order.
    auto stats = allocate_in_orders<test_registry, classes>(
        sampled_orders(12, 200), [&](const auto& comp) {
            BOOST_TEST(check(comp[m_P])->slots[0] == 5u);
            BOOST_TEST(check(comp[m_X])->slots[0] == 4u);
            BOOST_TEST(get_class<X>(comp)->vtbl.size() == 2u);
            BOOST_TEST(get_class<D>(comp)->vtbl.size() == 6u);
        });

    expect_total(stats, 51);
}

namespace several_parameters_per_class {

/*
A: two unary methods and a binary method on (A, A)
|
B1
*/

struct A {
    virtual ~A() = default;
};
struct B : A {};

using classes = boost::mp11::mp_list<A, B>;
BOOST_OPENMETHOD_REGISTER(use_classes<A, B, lattices_registry>);

} // namespace several_parameters_per_class

BOOST_AUTO_TEST_CASE(test_assign_slots_several_parameters_per_class) {
    using test_registry = lattices_registry;
    using namespace several_parameters_per_class;

    USE_METHOD_N(A, 1);
    USE_METHOD_N(A, 2);
    USE_METHOD2(A, A);
    ADD_METHOD(B);

    auto stats = allocate_in_orders<test_registry, classes>(
        all_orders(2), [&](const auto& comp) {
            BOOST_TEST(get_class<A>(comp)->vtbl.size() == 4u);
            BOOST_TEST(get_class<B>(comp)->vtbl.size() == 5u);
            BOOST_TEST(check(comp[m_B])->slots[0] == 4u);
        });

    expect_total(stats, 9);
}

namespace method_less_classes {

/*
A1   R
|    |
B    S1
|\   |
C1 D1 T1
*/

struct A {
    virtual ~A() = default;
};
struct B : A {};
struct C : B {};
struct D : B {};
struct R {
    virtual ~R() = default;
};
struct S : R {};
struct T : S {};

using classes = boost::mp11::mp_list<A, B, C, D, R, S, T>;
BOOST_OPENMETHOD_REGISTER(use_classes<A, B, C, D, R, S, T, lattices_registry>);

} // namespace method_less_classes

BOOST_AUTO_TEST_CASE(test_assign_slots_method_less_classes) {
    using test_registry = lattices_registry;
    using namespace method_less_classes;

    USE_METHOD(A);
    ADD_METHOD(C);
    ADD_METHOD(D);
    ADD_METHOD(S);
    ADD_METHOD(T);

    auto stats = allocate_in_orders<test_registry, classes>(
        all_orders(7), [&](const auto& comp) {
            BOOST_TEST(get_class<B>(comp)->vtbl.size() == 1u);
            BOOST_TEST(check(comp[m_C])->slots[0] == 1u);
            BOOST_TEST(check(comp[m_D])->slots[0] == 1u);
            BOOST_TEST(get_class<R>(comp)->vtbl.size() == 0u);
            BOOST_TEST(check(comp[m_S])->slots[0] == 0u);
            BOOST_TEST(check(comp[m_T])->slots[0] == 1u);
        });

    expect_total(stats, 9);
}

namespace components {

/*
A1  X1   and a binary method on (A, X)
|   |
B1  Y1
*/

struct A {
    virtual ~A() = default;
};
struct B : A {};
struct X {
    virtual ~X() = default;
};
struct Y : X {};

using classes = boost::mp11::mp_list<A, B, X, Y>;
BOOST_OPENMETHOD_REGISTER(use_classes<A, B, X, Y, lattices_registry>);

} // namespace components

BOOST_AUTO_TEST_CASE(test_assign_slots_components) {
    using test_registry = lattices_registry;
    using namespace components;

    USE_METHOD(A);
    ADD_METHOD(B);
    USE_METHOD(X);
    ADD_METHOD(Y);
    USE_METHOD2(A, X);

    // Unrelated hierarchies do not see each other: both start at slot 0.
    auto stats = allocate_in_orders<test_registry, classes>(
        all_orders(4), [&](const auto& comp) {
            BOOST_TEST(get_class<A>(comp)->vtbl.size() == 2u);
            BOOST_TEST(get_class<B>(comp)->vtbl.size() == 3u);
            BOOST_TEST(get_class<X>(comp)->vtbl.size() == 2u);
            BOOST_TEST(get_class<Y>(comp)->vtbl.size() == 3u);
            BOOST_TEST(check(comp[m_B])->slots[0] == 2u);
            BOOST_TEST(check(comp[m_Y])->slots[0] == 2u);
        });

    expect_total(stats, 10);
}

// ----------------------------------------------------------------------------
// Real hierarchies, in a sample of registration orders.

// The stream classes of the C++ standard library, per the synopsis in
// [iostream.forward.overview]: basic_ios is a virtual base of basic_istream and
// basic_ostream, which basic_iostream joins back; the stream buffers are a
// separate tree.
namespace iso_cpp_streams {

struct ios_base {
    virtual ~ios_base() = default;
};
struct basic_ios : ios_base {};
struct basic_istream : virtual basic_ios {};
struct basic_ostream : virtual basic_ios {};
struct basic_iostream : basic_istream, basic_ostream {};
struct basic_ifstream : basic_istream {};
struct basic_ofstream : basic_ostream {};
struct basic_fstream : basic_iostream {};
struct basic_istringstream : basic_istream {};
struct basic_ostringstream : basic_ostream {};
struct basic_stringstream : basic_iostream {};
struct basic_ispanstream : basic_istream {};
struct basic_ospanstream : basic_ostream {};
struct basic_spanstream : basic_iostream {};
struct basic_osyncstream : basic_ostream {};

struct basic_streambuf {
    virtual ~basic_streambuf() = default;
};
struct basic_filebuf : basic_streambuf {};
struct basic_stringbuf : basic_streambuf {};
struct basic_spanbuf : basic_streambuf {};
struct basic_syncbuf : basic_streambuf {};

using classes = boost::mp11::mp_list<
    ios_base, basic_ios, basic_istream, basic_ostream, basic_iostream,
    basic_ifstream, basic_ofstream, basic_fstream, basic_istringstream,
    basic_ostringstream, basic_stringstream, basic_ispanstream,
    basic_ospanstream, basic_spanstream, basic_osyncstream, basic_streambuf,
    basic_filebuf, basic_stringbuf, basic_spanbuf, basic_syncbuf>;
BOOST_OPENMETHOD_REGISTER(
    boost::mp11::mp_apply<
        use_classes, boost::mp11::mp_push_back<classes, hierarchies_registry>>);

} // namespace iso_cpp_streams

BOOST_AUTO_TEST_CASE(test_assign_slots_iso_cpp_streams) {
    using test_registry = hierarchies_registry;
    using namespace iso_cpp_streams;

    USE_METHOD(ios_base);
    USE_METHOD(basic_ios);
    USE_METHOD(basic_istream);
    USE_METHOD(basic_ostream);
    USE_METHOD(basic_iostream);
    USE_METHOD(basic_ifstream);
    USE_METHOD(basic_ofstream);
    USE_METHOD(basic_fstream);
    USE_METHOD(basic_istringstream);
    USE_METHOD(basic_ostringstream);
    USE_METHOD(basic_stringstream);
    USE_METHOD(basic_ispanstream);
    USE_METHOD(basic_ospanstream);
    USE_METHOD(basic_spanstream);
    USE_METHOD(basic_osyncstream);
    USE_METHOD(basic_streambuf);
    USE_METHOD(basic_filebuf);
    USE_METHOD(basic_stringbuf);
    USE_METHOD(basic_spanbuf);
    USE_METHOD(basic_syncbuf);
    USE_METHOD2(basic_ostream, basic_streambuf);

    // The old allocator: 95 to 97 over these orders.
    auto stats = allocate_in_orders<test_registry, classes>(
        sampled_orders(boost::mp11::mp_size<classes>::value, 64),
        [](const auto&) {});

    expect_total(stats, 88);
}

// The servers and request handlers of CPython's socketserver module
// (Lib/socketserver.py, PSF License 2.0): two mix-ins combined with each of
// four servers, and a separate handler tree.
namespace socketserver {

struct BaseServer {
    virtual ~BaseServer() = default;
};
struct TCPServer : BaseServer {};
struct UDPServer : TCPServer {};
struct UnixStreamServer : TCPServer {};
struct UnixDatagramServer : UDPServer {};
struct ThreadingMixIn {
    virtual ~ThreadingMixIn() = default;
};
struct ForkingMixIn {
    virtual ~ForkingMixIn() = default;
};
struct ThreadingTCPServer : ThreadingMixIn, TCPServer {};
struct ThreadingUDPServer : ThreadingMixIn, UDPServer {};
struct ForkingTCPServer : ForkingMixIn, TCPServer {};
struct ForkingUDPServer : ForkingMixIn, UDPServer {};
struct ThreadingUnixStreamServer : ThreadingMixIn, UnixStreamServer {};
struct ThreadingUnixDatagramServer : ThreadingMixIn, UnixDatagramServer {};
struct ForkingUnixStreamServer : ForkingMixIn, UnixStreamServer {};
struct ForkingUnixDatagramServer : ForkingMixIn, UnixDatagramServer {};

struct BaseRequestHandler {
    virtual ~BaseRequestHandler() = default;
};
struct StreamRequestHandler : BaseRequestHandler {};
struct DatagramRequestHandler : BaseRequestHandler {};

using classes = boost::mp11::mp_list<
    BaseServer, TCPServer, UDPServer, UnixStreamServer, UnixDatagramServer,
    ThreadingMixIn, ForkingMixIn, ThreadingTCPServer, ThreadingUDPServer,
    ForkingTCPServer, ForkingUDPServer, ThreadingUnixStreamServer,
    ThreadingUnixDatagramServer, ForkingUnixStreamServer,
    ForkingUnixDatagramServer, BaseRequestHandler, StreamRequestHandler,
    DatagramRequestHandler>;
BOOST_OPENMETHOD_REGISTER(
    boost::mp11::mp_apply<
        use_classes, boost::mp11::mp_push_back<classes, hierarchies_registry>>);

} // namespace socketserver

BOOST_AUTO_TEST_CASE(test_assign_slots_socketserver) {
    using test_registry = hierarchies_registry;
    using namespace socketserver;

    USE_METHOD(BaseServer);
    USE_METHOD(TCPServer);
    USE_METHOD(UDPServer);
    USE_METHOD(UnixStreamServer);
    USE_METHOD(UnixDatagramServer);
    USE_METHOD(ThreadingMixIn);
    USE_METHOD(ForkingMixIn);
    USE_METHOD(ThreadingTCPServer);
    USE_METHOD(ThreadingUDPServer);
    USE_METHOD(ForkingTCPServer);
    USE_METHOD(ForkingUDPServer);
    USE_METHOD(ThreadingUnixStreamServer);
    USE_METHOD(ThreadingUnixDatagramServer);
    USE_METHOD(ForkingUnixStreamServer);
    USE_METHOD(ForkingUnixDatagramServer);
    USE_METHOD(BaseRequestHandler);
    USE_METHOD(StreamRequestHandler);
    USE_METHOD(DatagramRequestHandler);
    USE_METHOD2(BaseServer, BaseRequestHandler);

    // The old allocator: 76 to 84 over these orders. Its best order beats
    // this by one entry: the greedy choice is not always the optimum.
    auto stats = allocate_in_orders<test_registry, classes>(
        sampled_orders(boost::mp11::mp_size<classes>::value, 64),
        [](const auto&) {});

    expect_total(stats, 77);
}

// Django's class-based generic views (django/views/generic/{base, detail,
// list, edit, dates}.py, BSD-3-Clause): mix-ins combined many times over.
// ContextMixin is reached twice by ModelFormMixin and BaseDeleteView, so it is
// a virtual base.
namespace django_views {

struct View {
    virtual ~View() = default;
};
struct ContextMixin {
    virtual ~ContextMixin() = default;
};
struct TemplateResponseMixin {
    virtual ~TemplateResponseMixin() = default;
};
struct TemplateView : TemplateResponseMixin, virtual ContextMixin, View {};
struct RedirectView : View {};

struct SingleObjectMixin : virtual ContextMixin {};
struct BaseDetailView : SingleObjectMixin, View {};
struct SingleObjectTemplateResponseMixin : TemplateResponseMixin {};
struct DetailView : SingleObjectTemplateResponseMixin, BaseDetailView {};

struct MultipleObjectMixin : virtual ContextMixin {};
struct BaseListView : MultipleObjectMixin, View {};
struct MultipleObjectTemplateResponseMixin : TemplateResponseMixin {};
struct ListView : MultipleObjectTemplateResponseMixin, BaseListView {};

struct FormMixin : virtual ContextMixin {};
struct ModelFormMixin : FormMixin, SingleObjectMixin {};
struct ProcessFormView : View {};
struct BaseFormView : FormMixin, ProcessFormView {};
struct FormView : TemplateResponseMixin, BaseFormView {};
struct BaseCreateView : ModelFormMixin, ProcessFormView {};
struct CreateView : SingleObjectTemplateResponseMixin, BaseCreateView {};
struct BaseUpdateView : ModelFormMixin, ProcessFormView {};
struct UpdateView : SingleObjectTemplateResponseMixin, BaseUpdateView {};
struct DeletionMixin {
    virtual ~DeletionMixin() = default;
};
struct BaseDeleteView : DeletionMixin, FormMixin, BaseDetailView {};
struct DeleteView : SingleObjectTemplateResponseMixin, BaseDeleteView {};

struct YearMixin {
    virtual ~YearMixin() = default;
};
struct MonthMixin {
    virtual ~MonthMixin() = default;
};
struct DayMixin {
    virtual ~DayMixin() = default;
};
struct WeekMixin {
    virtual ~WeekMixin() = default;
};
struct DateMixin {
    virtual ~DateMixin() = default;
};
struct BaseDateListView : MultipleObjectMixin, DateMixin, View {};
struct BaseArchiveIndexView : BaseDateListView {};
struct ArchiveIndexView :
    MultipleObjectTemplateResponseMixin,
    BaseArchiveIndexView {};
struct BaseYearArchiveView : YearMixin, BaseDateListView {};
struct YearArchiveView :
    MultipleObjectTemplateResponseMixin,
    BaseYearArchiveView {};
struct BaseMonthArchiveView : YearMixin, MonthMixin, BaseDateListView {};
struct MonthArchiveView :
    MultipleObjectTemplateResponseMixin,
    BaseMonthArchiveView {};
struct BaseWeekArchiveView : YearMixin, WeekMixin, BaseDateListView {};
struct WeekArchiveView :
    MultipleObjectTemplateResponseMixin,
    BaseWeekArchiveView {};
struct BaseDayArchiveView :
    YearMixin,
    MonthMixin,
    DayMixin,
    BaseDateListView {};
struct DayArchiveView :
    MultipleObjectTemplateResponseMixin,
    BaseDayArchiveView {};
struct BaseTodayArchiveView : BaseDayArchiveView {};
struct TodayArchiveView :
    MultipleObjectTemplateResponseMixin,
    BaseTodayArchiveView {};
struct BaseDateDetailView :
    YearMixin,
    MonthMixin,
    DayMixin,
    DateMixin,
    BaseDetailView {};
struct DateDetailView :
    SingleObjectTemplateResponseMixin,
    BaseDateDetailView {};

using classes = boost::mp11::mp_list<
    View, ContextMixin, TemplateResponseMixin, TemplateView, RedirectView,
    SingleObjectMixin, BaseDetailView, SingleObjectTemplateResponseMixin,
    DetailView, MultipleObjectMixin, BaseListView,
    MultipleObjectTemplateResponseMixin, ListView, FormMixin, ModelFormMixin,
    ProcessFormView, BaseFormView, FormView, BaseCreateView, CreateView,
    BaseUpdateView, UpdateView, DeletionMixin, BaseDeleteView, DeleteView,
    YearMixin, MonthMixin, DayMixin, WeekMixin, DateMixin, BaseDateListView,
    BaseArchiveIndexView, ArchiveIndexView, BaseYearArchiveView,
    YearArchiveView, BaseMonthArchiveView, MonthArchiveView,
    BaseWeekArchiveView, WeekArchiveView, BaseDayArchiveView, DayArchiveView,
    BaseTodayArchiveView, TodayArchiveView, BaseDateDetailView, DateDetailView>;
BOOST_OPENMETHOD_REGISTER(
    boost::mp11::mp_apply<
        use_classes, boost::mp11::mp_push_back<classes, hierarchies_registry>>);

} // namespace django_views

BOOST_AUTO_TEST_CASE(test_assign_slots_django_views) {
    using test_registry = hierarchies_registry;
    using namespace django_views;

    USE_METHOD(View);
    USE_METHOD(ContextMixin);
    USE_METHOD(TemplateResponseMixin);
    USE_METHOD(TemplateView);
    USE_METHOD(RedirectView);
    USE_METHOD(SingleObjectMixin);
    USE_METHOD(BaseDetailView);
    USE_METHOD(SingleObjectTemplateResponseMixin);
    USE_METHOD(DetailView);
    USE_METHOD(MultipleObjectMixin);
    USE_METHOD(BaseListView);
    USE_METHOD(MultipleObjectTemplateResponseMixin);
    USE_METHOD(ListView);
    USE_METHOD(FormMixin);
    USE_METHOD(ModelFormMixin);
    USE_METHOD(ProcessFormView);
    USE_METHOD(BaseFormView);
    USE_METHOD(FormView);
    USE_METHOD(BaseCreateView);
    USE_METHOD(CreateView);
    USE_METHOD(BaseUpdateView);
    USE_METHOD(UpdateView);
    USE_METHOD(DeletionMixin);
    USE_METHOD(BaseDeleteView);
    USE_METHOD(DeleteView);
    USE_METHOD(YearMixin);
    USE_METHOD(MonthMixin);
    USE_METHOD(DayMixin);
    USE_METHOD(WeekMixin);
    USE_METHOD(DateMixin);
    USE_METHOD(BaseDateListView);
    USE_METHOD(BaseArchiveIndexView);
    USE_METHOD(ArchiveIndexView);
    USE_METHOD(BaseYearArchiveView);
    USE_METHOD(YearArchiveView);
    USE_METHOD(BaseMonthArchiveView);
    USE_METHOD(MonthArchiveView);
    USE_METHOD(BaseWeekArchiveView);
    USE_METHOD(WeekArchiveView);
    USE_METHOD(BaseDayArchiveView);
    USE_METHOD(DayArchiveView);
    USE_METHOD(BaseTodayArchiveView);
    USE_METHOD(TodayArchiveView);
    USE_METHOD(BaseDateDetailView);
    USE_METHOD(DateDetailView);

    // The old allocator: 298 to 420 over these orders; its best order beats
    // this by five entries.
    auto stats = allocate_in_orders<test_registry, classes>(
        sampled_orders(boost::mp11::mp_size<classes>::value, 64),
        [](const auto&) {});

    expect_total(stats, 303);
}

// The standardized condition types of ANSI Common Lisp (section 9.1.1, and
// the class precedence lists in the type dictionary): a wide tree with four
// multiply-derived types. `condition` and `error` are virtual bases, for
// `simple-error`, `simple-warning`, `simple-type-error` and `reader-error`.
namespace common_lisp {

struct condition {
    virtual ~condition() = default;
};
struct serious_condition : virtual condition {};
struct warning : virtual condition {};
struct simple_condition : virtual condition {};
struct error : serious_condition {};
struct storage_condition : serious_condition {};
struct style_warning : warning {};
struct simple_error : simple_condition, error {};
struct simple_warning : simple_condition, warning {};
struct type_error : error {};
struct simple_type_error : simple_condition, type_error {};
struct arithmetic_error : error {};
struct division_by_zero : arithmetic_error {};
struct floating_point_inexact : arithmetic_error {};
struct floating_point_invalid_operation : arithmetic_error {};
struct floating_point_overflow : arithmetic_error {};
struct floating_point_underflow : arithmetic_error {};
struct cell_error : error {};
struct unbound_variable : cell_error {};
struct unbound_slot : cell_error {};
struct undefined_function : cell_error {};
struct control_error : error {};
struct file_error : error {};
struct package_error : error {};
struct parse_error : virtual error {};
struct print_not_readable : error {};
struct program_error : error {};
struct stream_error : virtual error {};
struct end_of_file : stream_error {};
struct reader_error : parse_error, stream_error {};

using classes = boost::mp11::mp_list<
    condition, serious_condition, warning, simple_condition, error,
    storage_condition, style_warning, simple_error, simple_warning, type_error,
    simple_type_error, arithmetic_error, division_by_zero,
    floating_point_inexact, floating_point_invalid_operation,
    floating_point_overflow, floating_point_underflow, cell_error,
    unbound_variable, unbound_slot, undefined_function, control_error,
    file_error, package_error, parse_error, print_not_readable, program_error,
    stream_error, end_of_file, reader_error>;
BOOST_OPENMETHOD_REGISTER(
    boost::mp11::mp_apply<
        use_classes, boost::mp11::mp_push_back<classes, hierarchies_registry>>);

} // namespace common_lisp

BOOST_AUTO_TEST_CASE(test_assign_slots_common_lisp_conditions) {
    using test_registry = hierarchies_registry;
    using namespace common_lisp;

    USE_METHOD(condition);
    USE_METHOD(serious_condition);
    USE_METHOD(warning);
    USE_METHOD(simple_condition);
    USE_METHOD(error);
    USE_METHOD(storage_condition);
    USE_METHOD(style_warning);
    USE_METHOD(simple_error);
    USE_METHOD(simple_warning);
    USE_METHOD(type_error);
    USE_METHOD(simple_type_error);
    USE_METHOD(arithmetic_error);
    USE_METHOD(division_by_zero);
    USE_METHOD(floating_point_inexact);
    USE_METHOD(floating_point_invalid_operation);
    USE_METHOD(floating_point_overflow);
    USE_METHOD(floating_point_underflow);
    USE_METHOD(cell_error);
    USE_METHOD(unbound_variable);
    USE_METHOD(unbound_slot);
    USE_METHOD(undefined_function);
    USE_METHOD(control_error);
    USE_METHOD(file_error);
    USE_METHOD(package_error);
    USE_METHOD(parse_error);
    USE_METHOD(print_not_readable);
    USE_METHOD(program_error);
    USE_METHOD(stream_error);
    USE_METHOD(end_of_file);
    USE_METHOD(reader_error);

    // The old allocator: 131 to 143 over these orders.
    auto stats = allocate_in_orders<test_registry, classes>(
        sampled_orders(boost::mp11::mp_size<classes>::value, 64),
        [](const auto&) {});

    expect_total(stats, 126);
}

// The abstract base classes of CPython's collections.abc module (as listed in
// the library reference, PSF License 2.0). Iterable is reached twice by
// Sequence, and Sized by the mapping views, so both are virtual bases.
namespace collections_abc {

struct Container {
    virtual ~Container() = default;
};
struct Hashable {
    virtual ~Hashable() = default;
};
struct Iterable {
    virtual ~Iterable() = default;
};
struct Iterator : Iterable {};
struct Reversible : virtual Iterable {};
struct Generator : Iterator {};
struct Sized {
    virtual ~Sized() = default;
};
struct Callable {
    virtual ~Callable() = default;
};
struct Collection : virtual Sized, virtual Iterable, Container {};
struct Sequence : Reversible, Collection {};
struct MutableSequence : Sequence {};
struct ByteString : Sequence {};
struct Set : Collection {};
struct MutableSet : Set {};
struct Mapping : Collection {};
struct MutableMapping : Mapping {};
struct MappingView : virtual Sized {};
struct ItemsView : MappingView, Set {};
struct KeysView : MappingView, Set {};
struct ValuesView : MappingView, Collection {};
struct Awaitable {
    virtual ~Awaitable() = default;
};
struct Coroutine : Awaitable {};
struct AsyncIterable {
    virtual ~AsyncIterable() = default;
};
struct AsyncIterator : AsyncIterable {};
struct AsyncGenerator : AsyncIterator {};
struct Buffer {
    virtual ~Buffer() = default;
};

using classes = boost::mp11::mp_list<
    Container, Hashable, Iterable, Iterator, Reversible, Generator, Sized,
    Callable, Collection, Sequence, MutableSequence, ByteString, Set,
    MutableSet, Mapping, MutableMapping, MappingView, ItemsView, KeysView,
    ValuesView, Awaitable, Coroutine, AsyncIterable, AsyncIterator,
    AsyncGenerator, Buffer>;
BOOST_OPENMETHOD_REGISTER(
    boost::mp11::mp_apply<
        use_classes, boost::mp11::mp_push_back<classes, hierarchies_registry>>);

} // namespace collections_abc

BOOST_AUTO_TEST_CASE(test_assign_slots_collections_abc) {
    using test_registry = hierarchies_registry;
    using namespace collections_abc;

    USE_METHOD(Container);
    USE_METHOD(Hashable);
    USE_METHOD(Iterable);
    USE_METHOD(Iterator);
    USE_METHOD(Reversible);
    USE_METHOD(Generator);
    USE_METHOD(Sized);
    USE_METHOD(Callable);
    USE_METHOD(Collection);
    USE_METHOD(Sequence);
    USE_METHOD(MutableSequence);
    USE_METHOD(ByteString);
    USE_METHOD(Set);
    USE_METHOD(MutableSet);
    USE_METHOD(Mapping);
    USE_METHOD(MutableMapping);
    USE_METHOD(MappingView);
    USE_METHOD(ItemsView);
    USE_METHOD(KeysView);
    USE_METHOD(ValuesView);
    USE_METHOD(Awaitable);
    USE_METHOD(Coroutine);
    USE_METHOD(AsyncIterable);
    USE_METHOD(AsyncIterator);
    USE_METHOD(AsyncGenerator);
    USE_METHOD(Buffer);

    // The old allocator: 102 to 114 over these orders.
    auto stats = allocate_in_orders<test_registry, classes>(
        sampled_orders(boost::mp11::mp_size<classes>::value, 64),
        [](const auto&) {});

    expect_total(stats, 96);
}

// ============================================================================
// Test finalize.

BOOST_AUTO_TEST_CASE(test_finalize_clears_vptr_vector) {
    using test_registry = test_registry_<__COUNTER__>;

    struct A {
        virtual ~A() = default;
    };
    struct B : A {};

    // Function-local static: registers on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<A, B, test_registry> BOOST_OPENMETHOD_GENSYM;
    (void)method<A, auto(virtual_<A&>)->void, test_registry>::fn;

    initialize<test_registry>();

    auto& vptrs = test_registry::state<policies::vptr_vector>().vptrs;
    BOOST_TEST(!vptrs.empty());

    // The vptr policy provides a finalize() (portable across MSVC/non-MSVC).
    static_assert(detail::has_finalize<
                  test_registry::policy<policies::vptr>, const std::tuple<>&>);

    finalize<test_registry>();
    BOOST_TEST(vptrs.empty()); // finalize cleared the vector
}

BOOST_AUTO_TEST_CASE(test_registries_do_not_share_vptr_state) {
    // Two distinct registries, both using vptr_vector, must each have their own
    // vptr_vector state; they do not share it.
    using registry1 = test_registry_<__COUNTER__>;
    using registry2 = test_registry_<__COUNTER__>;

    static_assert(!std::is_same_v<registry1, registry2>);

    struct A {
        virtual ~A() = default;
    };
    struct B : A {};

    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<A, B, registry1> BOOST_OPENMETHOD_GENSYM;
    static use_classes<A, B, registry2> BOOST_OPENMETHOD_GENSYM;
    (void)method<A, auto(virtual_<A&>)->void, registry1>::fn;
    (void)method<A, auto(virtual_<A&>)->void, registry2>::fn;

    initialize<registry1>();
    initialize<registry2>();

    auto& vptrs1 = registry1::state<policies::vptr_vector>().vptrs;
    auto& vptrs2 = registry2::state<policies::vptr_vector>().vptrs;

    // Distinct state objects.
    BOOST_TEST(
        static_cast<const void*>(&vptrs1) != static_cast<const void*>(&vptrs2));

    // Independent: clearing one registry's state leaves the other intact.
    BOOST_TEST(!vptrs1.empty());
    BOOST_TEST(!vptrs2.empty());
    finalize<registry1>();
    BOOST_TEST(vptrs1.empty());
    BOOST_TEST(!vptrs2.empty());
}
