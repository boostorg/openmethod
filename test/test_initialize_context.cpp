// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// The class range a policy's `initialize` receives is an *input* range. Its
// iterator makes the `InitializeClass` view on the fly, so dereferencing
// yields a value, not a reference: two iterators at the same position hand out
// distinct views, and the multipass guarantee a forward iterator owes does not
// hold. It used to advertise `forward_iterator_tag` anyway, which is a promise
// a generic algorithm is entitled to act on.
//
// Pinned here because nothing else in the tree looks at the category: the
// library's own policies only ever `++`, `!=` and dereference.

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE initialize_context
#include <boost/test/unit_test.hpp>

#include "test_util.hpp"

#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>

using namespace boost::openmethod;

// Rides along on a registry that keeps its usual vptr policy, purely to get
// hold of a Context.
struct context_checks {
    using category = context_checks;

    template<class Registry>
    struct fn {
        struct state {
            std::size_t classes_seen = 0;
            bool values_stable = false;
        };

        template<class Context, class... Options>
        static void initialize(
            const Context& ctx, const std::tuple<Options...>&) {
            using iterator = decltype(ctx.classes_begin());
            using traits = std::iterator_traits<iterator>;

            static_assert(std::is_same_v<
                          typename traits::iterator_category,
                          std::input_iterator_tag>);

            // Why it cannot be a forward iterator: `reference` is the value
            // type, so there is nothing for a second pass to refer back to.
            static_assert(
                std::is_same_v<
                    typename traits::reference, typename traits::value_type>);

            auto& st = Registry::template state<context_checks>();

            st.classes_seen = static_cast<std::size_t>(
                std::distance(ctx.classes_begin(), ctx.classes_end()));

            // Single-pass does not mean unstable: two iterators at the same
            // position describe the same class, even though they hand out
            // separate views of it.
            auto i = ctx.classes_begin();
            auto j = ctx.classes_begin();
            st.values_stable = i != ctx.classes_end() &&
                i->vptr() == j->vptr() && i->static_vptr() == j->static_vptr();
        }
    };
};

template<int N>
struct test_reg : test_registry_<N>::template with<context_checks> {};

using reg = test_reg<__COUNTER__>;

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};
struct Cat : Animal {};

struct BOOST_OPENMETHOD_ID(poke);

using poke = method<
    BOOST_OPENMETHOD_ID(poke), auto(virtual_<Animal&>)->std::string, reg>;

auto poke_animal(Animal&) -> std::string {
    return "silence";
}

BOOST_AUTO_TEST_CASE(the_class_range_is_an_input_range) {
    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<Animal, Dog, Cat, reg> BOOST_OPENMETHOD_GENSYM;
    static poke::override<poke_animal> BOOST_OPENMETHOD_GENSYM;

    initialize<reg>();

    auto& st = reg::state<context_checks>();
    BOOST_TEST(st.classes_seen == 3u);
    BOOST_TEST(st.values_stable);

    // And the registry works, so the policy really did run.
    Dog dog;
    BOOST_TEST(poke::fn(dog) == "silence");
}
