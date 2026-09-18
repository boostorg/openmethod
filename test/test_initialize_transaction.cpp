// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// initialize() is transactional: if anything between staging the new dispatch
// data and the commit point throws - a policy's initialize, or the trace
// write that announces the installation, which goes through the user-supplied
// `output` policy - the registry keeps the dispatch state it had before the
// call: the static v-table pointers, the `next` pointers, the dispatch data
// and every policy's state, instead of pointers into a vector that unwinding
// has freed. It is marked as not initialized, though, until a call succeeds.

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>
#include <boost/openmethod/policies/vptr_map.hpp>

#define BOOST_TEST_MODULE initialize_transaction
#include <boost/test/unit_test.hpp>

#include "test_util.hpp"

#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

using boost::mp11::mp_list;
using namespace boost::openmethod;

// A stateful policy whose `initialize` throws on demand. It writes to its
// state before throwing, so a rollback is observable there too; and it
// comes last in the policy list (see the static_assert below), so by the time
// it throws, the type_hash and vptr policies have written their new states.
struct explosive_policy {
    using category = explosive_policy;

    template<class Registry>
    struct fn {
        struct state {
            int generation = 0;
        };

        inline static bool armed = false;
        inline static int generations = 0;

        template<class Context, class... Options>
        static void initialize(const Context&, const std::tuple<Options...>&) {
            Registry::template state<explosive_policy>().generation =
                ++generations;

            if (armed) {
                throw std::runtime_error("boom");
            }
        }
    };
};

template<int N>
struct vector_registry :
    test_registry_<N>::template with<
        policies::runtime_checks, policies::throw_error_handler,
        explosive_policy> {};

template<int N>
struct map_registry :
    test_registry_<N>::template with<
        policies::runtime_checks, policies::throw_error_handler,
        policies::vptr_map<>, policies::indirect_vptr, explosive_policy> {};

template<int N>
using registries = mp_list<vector_registry<N>, map_registry<N>>;

template<class Registry>
constexpr bool explosive_comes_last =
    boost::mp11::mp_find<
        typename Registry::policy_list, explosive_policy>::value >
    boost::mp11::mp_find<
        typename Registry::policy_list,
        detail::find_first_derived_of<
            policies::vptr, typename Registry::policy_list>>::value;

static_assert(explosive_comes_last<vector_registry<0>>);
static_assert(explosive_comes_last<map_registry<0>>);

struct Animal {
    virtual ~Animal() = default;
};

struct Carnivore : Animal {};
struct Dog : Carnivore {};
struct Cat : Animal {};
struct Bird : Animal {};

struct BOOST_OPENMETHOD_ID(poke);

template<class Registry>
using poke = method<
    BOOST_OPENMETHOD_ID(poke), auto(virtual_<Animal&>)->std::string, Registry>;

template<class Registry>
auto poke_animal(Animal&) -> std::string {
    return "silence";
}

template<class Registry>
auto poke_dog(Dog& dog) -> std::string {
    return poke<Registry>::template next<poke_dog<Registry>>(dog) + " bark";
}

template<class Registry>
auto poke_carnivore(Carnivore& carnivore) -> std::string {
    return poke<Registry>::template next<poke_carnivore<Registry>>(carnivore) +
        " growl";
}

template<class Registry>
struct snapshot {
    using vptr_state =
        typename Registry::template policy<policies::vptr>::state;
    using type_hash = typename Registry::template policy<policies::type_hash>;
    using hash_state = typename type_hash::state;

    snapshot() :
        dispatch_data(Registry::state().dispatch_data.data()),
        dog_vptr(Registry::template static_vptr<Dog>),
        cat_vptr(Registry::template static_vptr<Cat>),
        next(poke<Registry>::template next<poke_dog<Registry>>),
        policies(Registry::state().policies) {
    }

    auto vptrs() -> decltype(auto) {
        return (detail::get<vptr_state>(policies).vptrs);
    }

    // Everything `fast_perfect_hash::initialize` writes: the factors and the
    // control table. `hash_range()` alone would miss a rollback that restored
    // the range but not the factors.
    auto hash() -> decltype(auto) {
        return (detail::get<hash_state>(policies));
    }

    const detail::word* dispatch_data;
    vptr_type dog_vptr;
    vptr_type cat_vptr;
    decltype(poke<Registry>::template next<poke_dog<Registry>>) next;
    decltype(Registry::state().policies) policies;
};

BOOST_AUTO_TEST_CASE_TEMPLATE(
    failed_reinitialize_keeps_previous_state, Registry,
    registries<__COUNTER__>) {
    using explosive = typename explosive_policy::template fn<
        typename Registry::registry_type>;
    using vptr_state = typename snapshot<Registry>::vptr_state;

    // Dog is registered here with Animal as its direct base, although it
    // really derives from Carnivore. The missing edge is added between the two
    // initializes, below.
    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<Animal, Carnivore, Cat, Registry>
        BOOST_OPENMETHOD_GENSYM;
    static use_classes<Animal, Dog, Registry> BOOST_OPENMETHOD_GENSYM;
    static typename poke<Registry>::template override<poke_animal<Registry>>
        BOOST_OPENMETHOD_GENSYM;
    static typename poke<Registry>::template override<poke_dog<Registry>>
        BOOST_OPENMETHOD_GENSYM;
    static typename poke<Registry>::template override<poke_carnivore<Registry>>
        BOOST_OPENMETHOD_GENSYM;

    Dog dog;
    Cat cat;
    auto& st = Registry::state();

    initialize<Registry>();
    BOOST_TEST(st.initialized);
    BOOST_TEST(poke<Registry>::fn(dog) == "silence bark");
    BOOST_TEST(poke<Registry>::fn(cat) == "silence");
    BOOST_TEST(Registry::template state<explosive_policy>().generation == 1);

    snapshot<Registry> before;

    // Perturb the input of the call that is about to fail. Without this it
    // would see exactly what the successful call saw, and recompute
    // bit-identical values for everything compared below - `fast_perfect_hash`
    // re-seeds a fixed PRNG over the same class set, and `next<poke_dog>`
    // resolves to the same overrider - so the assertions would hold whether or
    // not the transaction rolled anything back. These registrars are
    // function-local statics: they register on first pass through the
    // declaration, here, not before main. `Bird` changes the class set that
    // the hash factors, the control table and the v-table pointers are
    // computed from; the Carnivore edge inserts `poke_carnivore` between
    // `poke_dog` and `poke_animal`, changing what `next<poke_dog>` resolves
    // to. The final initialize below observes both.
    // BOOST_OPENMETHOD_REGISTER is now `inline`, which is illegal at block
    // scope, so it's spelled out here (same function-local-static reasoning
    // as above).
    static use_classes<Animal, Bird, Registry> BOOST_OPENMETHOD_GENSYM;
    static use_classes<Carnivore, Dog, Registry> BOOST_OPENMETHOD_GENSYM;

    explosive::armed = true;
    BOOST_CHECK_THROW(initialize<Registry>(), std::runtime_error);
    explosive::armed = false;

    // Not initialized, but everything the previous call installed is still
    // there, and consistent...
    BOOST_TEST(!st.initialized);
    BOOST_TEST(st.dispatch_data.data() == before.dispatch_data);
    BOOST_TEST(Registry::template static_vptr<Dog> == before.dog_vptr);
    BOOST_TEST(Registry::template static_vptr<Cat> == before.cat_vptr);
    // Parenthesized: on failure Boost.Test would print the operands, and
    // streaming a function pointer is a Microsoft extension that clang-cl
    // rejects under /WX (-Wmicrosoft-cast).
    BOOST_TEST(
        (poke<Registry>::template next<poke_dog<Registry>> == before.next));
    // Parenthesized for the same reason: Boost.Test cannot print hash factors
    // or vectors of type ids.
    auto& hash =
        detail::get<typename snapshot<Registry>::hash_state>(st.policies);
    BOOST_TEST((hash.fn.mult == before.hash().fn.mult));
    BOOST_TEST((hash.fn.shift == before.hash().fn.shift));
    BOOST_TEST((hash.fn.min_value == before.hash().fn.min_value));
    BOOST_TEST((hash.fn.max_value == before.hash().fn.max_value));
    BOOST_TEST((hash.control == before.hash().control));
    BOOST_TEST((detail::get<vptr_state>(st.policies).vptrs == before.vptrs()));
    // ...including the state of the policy that threw, after writing to it.
    BOOST_TEST(Registry::template state<explosive_policy>().generation == 1);

    // ...but dispatch is refused until an initialize() succeeds.
    BOOST_CHECK_THROW(poke<Registry>::fn(dog), not_initialized);

    // A successful call installs what the failed one would have: the
    // perturbation is visible in the result, which confirms that the
    // assertions above compared values that really do differ between the two
    // calls.
    initialize<Registry>();
    BOOST_TEST(st.initialized);
    BOOST_TEST(poke<Registry>::fn(dog) == "silence growl bark");
    BOOST_TEST(poke<Registry>::fn(cat) == "silence");
    BOOST_TEST(
        (poke<Registry>::template next<poke_dog<Registry>> != before.next));
    BOOST_TEST((hash.control != before.hash().control));
    BOOST_TEST(Registry::template state<explosive_policy>().generation == 3);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    failed_first_initialize_leaves_registry_clean, Registry,
    registries<__COUNTER__>) {
    using explosive = typename explosive_policy::template fn<
        typename Registry::registry_type>;
    using vptr_state = typename snapshot<Registry>::vptr_state;

    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<Animal, Dog, Cat, Registry> BOOST_OPENMETHOD_GENSYM;
    static typename poke<Registry>::template override<poke_animal<Registry>>
        BOOST_OPENMETHOD_GENSYM;
    static typename poke<Registry>::template override<poke_dog<Registry>>
        BOOST_OPENMETHOD_GENSYM;

    Dog dog;
    auto& st = Registry::state();

    explosive::armed = true;
    BOOST_CHECK_THROW(initialize<Registry>(), std::runtime_error);
    explosive::armed = false;

    BOOST_TEST(!st.initialized);
    BOOST_TEST(st.dispatch_data.empty());
    BOOST_TEST(Registry::template static_vptr<Dog> == nullptr);
    BOOST_TEST(detail::get<vptr_state>(st.policies).vptrs.empty());
    BOOST_TEST(Registry::template state<explosive_policy>().generation == 0);
    BOOST_CHECK_THROW(poke<Registry>::fn(dog), not_initialized);

    // finalize() has nothing to undo, and must not mind.
    finalize<Registry>();
    BOOST_TEST(!st.initialized);

    initialize<Registry>();
    BOOST_TEST(st.initialized);
    BOOST_TEST(poke<Registry>::fn(dog) == "silence bark");
}

// The other way an initialize can fail after the policies have run: the trace
// goes through the `output` policy, which is user code, so the statement that
// announces the installation can throw. It sits before the commit, and must
// stay there - after it, the policies would keep the v-table pointers they
// just read out of the staging vector, which unwinding frees.

// Discards what it is given, and throws once, on the message named in `trap`.
struct trapping_stream {
    static inline const char* trap = nullptr;

    void write(const char* str) {
        if (trap != nullptr && std::strstr(str, trap) != nullptr) {
            trap = nullptr;
            throw std::runtime_error("output");
        }
    }

    auto is_on() const -> bool {
        return true;
    }
};

inline auto operator<<(trapping_stream& os, const char* str)
    -> trapping_stream& {
    os.write(str);
    return os;
}

inline auto operator<<(trapping_stream& os, const std::string_view&)
    -> trapping_stream& {
    return os;
}

inline auto operator<<(trapping_stream& os, const void*) -> trapping_stream& {
    return os;
}

inline auto operator<<(trapping_stream& os, void (*)()) -> trapping_stream& {
    return os;
}

inline auto operator<<(trapping_stream& os, std::size_t) -> trapping_stream& {
    return os;
}

struct trapping_output : policies::output {
    template<class Registry>
    struct fn {
        struct state {
            trapping_stream os;
        };

        static auto& stream() {
            return Registry::template state<trapping_output>().os;
        }
    };
};

template<int N>
struct tracing_registry :
    test_registry_<N>::template with<
        policies::runtime_checks, policies::throw_error_handler,
        trapping_output> {};

BOOST_AUTO_TEST_CASE(a_throwing_trace_does_not_commit) {
    using Registry = tracing_registry<__COUNTER__>;
    using vptr_state = typename snapshot<Registry>::vptr_state;

    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<Animal, Dog, Cat, Registry> BOOST_OPENMETHOD_GENSYM;
    static poke<Registry>::override<poke_animal<Registry>>
        BOOST_OPENMETHOD_GENSYM;
    static poke<Registry>::override<poke_dog<Registry>> BOOST_OPENMETHOD_GENSYM;

    Dog dog;
    auto& st = Registry::state();

    initialize<Registry>();
    BOOST_TEST(poke<Registry>::fn(dog) == "silence bark");

    snapshot<Registry> before;

    trapping_stream::trap = "Installing";
    BOOST_CHECK_THROW(initialize<Registry>(trace(true)), std::runtime_error);
    BOOST_TEST(trapping_stream::trap == nullptr); // it did throw there

    BOOST_TEST(!st.initialized);
    BOOST_TEST(st.dispatch_data.data() == before.dispatch_data);
    BOOST_TEST(Registry::template static_vptr<Dog> == before.dog_vptr);
    // The one that matters: had the policies been committed, these would be
    // pointers into the staging vector, which no longer exists.
    BOOST_TEST((detail::get<vptr_state>(st.policies).vptrs == before.vptrs()));

    initialize<Registry>();
    BOOST_TEST(st.initialized);
    BOOST_TEST(poke<Registry>::fn(dog) == "silence bark");
}

// Same again for the reporting. print(report) and print_slots() used to run
// after write_global_data() had committed, so a throw out of either - through
// the user's `output` policy, or out of the containers print_slots() builds -
// left the new tables installed while initialize() never reached
// `st.initialized = true`. That is a fourth outcome the exception-safety
// contract does not describe, and it makes the next call abort with
// `not_initialized` under runtime_checks even though the tables are fine.
BOOST_AUTO_TEST_CASE(a_throwing_report_does_not_commit) {
    using Registry = tracing_registry<__COUNTER__>;
    using vptr_state = typename snapshot<Registry>::vptr_state;

    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<Animal, Dog, Cat, Registry> BOOST_OPENMETHOD_GENSYM;
    static poke<Registry>::override<poke_animal<Registry>>
        BOOST_OPENMETHOD_GENSYM;
    static poke<Registry>::override<poke_dog<Registry>> BOOST_OPENMETHOD_GENSYM;

    Dog dog;
    auto& st = Registry::state();

    initialize<Registry>();
    BOOST_TEST(poke<Registry>::fn(dog) == "silence bark");

    snapshot<Registry> before;

    trapping_stream::trap = "Used slots";
    BOOST_CHECK_THROW(initialize<Registry>(trace(true)), std::runtime_error);
    BOOST_TEST(trapping_stream::trap == nullptr); // it did throw there

    BOOST_TEST(!st.initialized);
    BOOST_TEST(st.dispatch_data.data() == before.dispatch_data);
    BOOST_TEST(Registry::template static_vptr<Dog> == before.dog_vptr);
    BOOST_TEST((detail::get<vptr_state>(st.policies).vptrs == before.vptrs()));

    initialize<Registry>();
    BOOST_TEST(st.initialized);
    BOOST_TEST(poke<Registry>::fn(dog) == "silence bark");
}

// "The state the call found" is meant literally: it is not necessarily a state
// the registry can dispatch through. finalize() clears the dispatch data and
// every policy's state but leaves the classes' static_vptrs set - documented
// on static_vptr, which remains valid only until the next initialize() *or
// finalize()*. A failed initialize() after that restores exactly that
// half-torn-down state, which is why the guarantee is worded as preservation
// and not as consistency.
BOOST_AUTO_TEST_CASE_TEMPLATE(
    failed_initialize_after_finalize_restores_what_it_found, Registry,
    registries<__COUNTER__>) {
    using explosive = typename explosive_policy::template fn<
        typename Registry::registry_type>;
    using vptr_state = typename snapshot<Registry>::vptr_state;

    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<Animal, Dog, Cat, Registry> BOOST_OPENMETHOD_GENSYM;
    static typename poke<Registry>::template override<poke_animal<Registry>>
        BOOST_OPENMETHOD_GENSYM;
    static typename poke<Registry>::template override<poke_dog<Registry>>
        BOOST_OPENMETHOD_GENSYM;

    Dog dog;
    auto& st = Registry::state();

    initialize<Registry>();
    BOOST_TEST(poke<Registry>::fn(dog) == "silence bark");

    finalize<Registry>();
    BOOST_TEST(!st.initialized);
    BOOST_TEST(st.dispatch_data.empty());
    BOOST_TEST(detail::get<vptr_state>(st.policies).vptrs.empty());
    // Not cleared by finalize, and so still set here.
    auto dog_vptr_after_finalize = Registry::template static_vptr<Dog>;
    BOOST_TEST(dog_vptr_after_finalize != nullptr);

    explosive::armed = true;
    BOOST_CHECK_THROW(initialize<Registry>(), std::runtime_error);
    explosive::armed = false;

    // Everything is put back the way the failed call found it - torn down, not
    // consistent.
    BOOST_TEST(!st.initialized);
    BOOST_TEST(st.dispatch_data.empty());
    BOOST_TEST(detail::get<vptr_state>(st.policies).vptrs.empty());
    BOOST_TEST(Registry::template static_vptr<Dog> == dog_vptr_after_finalize);

    // And a successful call still recovers from it.
    initialize<Registry>();
    BOOST_TEST(st.initialized);
    BOOST_TEST(poke<Registry>::fn(dog) == "silence bark");
}
