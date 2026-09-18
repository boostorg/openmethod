// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// initialize() saves and restores the state of the policies it initializes,
// and only those. A state that no `initialize` writes to is configuration the
// caller owns, not derived data the call is about to replace, so a failed
// initialize() must leave it alone - the error handler is the case that
// matters, since it is called from inside the transaction window by design.
//
// Two consequences, both checked here: such a state survives a rollback, and
// it does not have to be copyable - which it would if the transaction copied
// the whole policy tuple, ruling out the std::ostringstream an `output` policy
// written to the documented state pattern naturally holds.

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE initialize_policy_state_scope
#include <boost/test/unit_test.hpp>

#include "test_util.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

using namespace boost::openmethod;

// Configuration: it has a `state`, and deliberately no `initialize`. The state
// is move-only, like the std::ostringstream it carries - the transaction must
// not require it to be copyable.
struct config_policy {
    using category = config_policy;

    template<class Registry>
    struct fn {
        struct state {
            state() = default;
            state(const state&) = delete;
            auto operator=(const state&) -> state& = delete;

            int generation = 0;
            std::ostringstream os;
        };
    };
};

// Writes to its own state and to the configuration policy's - the way an error
// handler called from inside the window changes the handler it installs - then
// throws on demand.
struct explosive_policy {
    using category = explosive_policy;

    template<class Registry>
    struct fn {
        struct state {
            int generation = 0;
        };

        inline static bool armed = false;

        template<class Context, class... Options>
        static void initialize(const Context&, const std::tuple<Options...>&) {
            ++Registry::template state<explosive_policy>().generation;
            ++Registry::template state<config_policy>().generation;

            if (armed) {
                throw std::runtime_error("boom");
            }
        }
    };
};

template<int N>
struct test_registry :
    test_registry_<N>::template with<config_policy, explosive_policy> {};

using test_reg = test_registry<__COUNTER__>;
using config_state = config_policy::fn<test_reg::registry_type>::state;
using explosive = explosive_policy::fn<test_reg::registry_type>;

// The point of the exercise: a policy that has no `initialize` may hold a
// state the transaction could not copy even if it wanted to. If this ever
// becomes copyable the test below stops proving anything.
static_assert(!std::is_copy_constructible_v<config_state>);
static_assert(!std::is_copy_assignable_v<config_state>);

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};

struct BOOST_OPENMETHOD_ID(poke);

using poke = method<
    BOOST_OPENMETHOD_ID(poke), auto(virtual_<Animal&>)->std::string, test_reg>;

auto poke_animal(Animal&) -> std::string {
    return "silence";
}

BOOST_AUTO_TEST_CASE(config_state_survives_a_failed_initialize) {
    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<Animal, Dog, test_reg> BOOST_OPENMETHOD_GENSYM;
    static poke::override<poke_animal> BOOST_OPENMETHOD_GENSYM;

    initialize<test_reg>();
    BOOST_TEST(test_reg::state<explosive_policy>().generation == 1);
    BOOST_TEST(test_reg::state<config_policy>().generation == 1);

    explosive::armed = true;
    BOOST_CHECK_THROW(initialize<test_reg>(), std::runtime_error);
    explosive::armed = false;

    // The initializing policy's own state is derived data: rolled back, so the
    // second run's increment is undone.
    BOOST_TEST(test_reg::state<explosive_policy>().generation == 1);

    // The configuration policy's is not: the write made inside the window
    // stands, exactly as an error handler that disarms itself before throwing
    // would expect.
    BOOST_TEST(test_reg::state<config_policy>().generation == 2);

    // And the registry still works.
    initialize<test_reg>();
    Dog dog;
    BOOST_TEST(poke::fn(dog) == "silence");
}
