// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// A class stops being registered when the registrar that added it is
// destroyed - which is what happens to every class a shared library
// contributes when the library is unloaded. The next initialize() must not
// leave that class' v-table pointer behind: it points into the dispatch data
// the commit frees, so a later dispatch on the dropped class would read freed
// memory, and, after a dlclose, jump into unloaded code.
//
// The classes here are dropped by letting a block-scoped `use_classes` object
// die, which runs the same registrar destructor a library unload runs, in a
// single process.

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/policies/vptr_map.hpp>

#define BOOST_TEST_MODULE initialize_dropped_class
#include <boost/test/unit_test.hpp>

#include "test_util.hpp"

#include <cstddef>
#include <string>
#include <type_traits>

using boost::mp11::mp_list;
using namespace boost::openmethod;

// Small, dense type ids: without a type_hash policy they are used as indices
// into vptr_vector's vector directly, so which slot a class occupies is exact
// rather than a function of the hash factors, which are re-drawn whenever the
// set of classes changes.
struct Animal {
    explicit Animal(std::size_t type) : type(type) {
    }

    virtual ~Animal() = default;

    static constexpr std::size_t static_type = 1;
    std::size_t type;
};

struct Dog : Animal {
    explicit Dog(std::size_t type = static_type) : Animal(type) {
    }

    static constexpr std::size_t static_type = 2;
};

// Tiger sits between Dog and Cat so that dropping it does not shrink the
// vector: Cat still requires a slot past Tiger's. A truncating resize would
// hide the bug.
struct Tiger : Animal {
    explicit Tiger(std::size_t type = static_type) : Animal(type) {
    }

    static constexpr std::size_t static_type = 3;
};

struct Cat : Animal {
    explicit Cat(std::size_t type = static_type) : Animal(type) {
    }

    static constexpr std::size_t static_type = 4;
};

namespace {

// Everything that is not in the hierarchy - methods, overriders, void, char -
// also goes through static_type<T>(). Give those ids of their own, still small
// so they cannot inflate the vector.
inline auto next_other_id() -> std::size_t {
    static std::size_t counter = 100;
    return ++counter;
}

template<typename T>
inline auto other_static_type() -> std::size_t {
    static std::size_t value = next_other_id();
    return value;
}

} // namespace

struct small_rtti : policies::rtti {
    template<class Registry>
    struct fn : defaults {
        template<class T>
        static constexpr bool is_polymorphic = std::is_base_of_v<Animal, T>;

        template<typename T>
        static auto static_type() -> type_id {
            if constexpr (is_polymorphic<T>) {
                return type_id(T::static_type);
            } else {
                return type_id(other_static_type<T>());
            }
        }

        template<typename T>
        static auto dynamic_type(const T& obj) -> type_id {
            if constexpr (is_polymorphic<T>) {
                return type_id(obj.type);
            } else {
                return type_id(other_static_type<T>());
            }
        }
    };
};

template<int N>
struct vector_registry :
    test_registry_<N>::template with<small_rtti>::template without<
        policies::type_hash> {};

template<int N>
struct indirect_vector_registry :
    vector_registry<N>::template with<policies::indirect_vptr> {};

// vptr_map already rebuilds its map and swaps it in, so it is the control:
// these cases pass on it before the fix as well as after.
template<int N>
struct map_registry :
    vector_registry<N>::template with<policies::vptr_map<>> {};

template<int N>
using registries =
    mp_list<vector_registry<N>, indirect_vector_registry<N>, map_registry<N>>;

struct BOOST_OPENMETHOD_ID(speak);

template<class Registry>
using speak = method<
    BOOST_OPENMETHOD_ID(speak), auto(virtual_<Animal&>)->std::string, Registry>;

template<class Registry>
auto speak_animal(Animal&) -> std::string {
    return "...";
}

// Tiger deliberately has no overrider of its own. An overrider registers
// itself through `override_aux::impl`, a *static* member whose instantiation
// the registrar object merely forces (core.hpp:2329-2332), so it lives until
// the program exits no matter how the object is scoped. Dropping Tiger's class
// while an overrider still named it would just make the next initialize()
// report `unknown class Tiger`. Tiger inherits Animal's overrider instead, and
// what this test watches is its *slot*, not its overrider.

template<class Container, class = void>
constexpr bool is_map = false;

template<class Container>
constexpr bool is_map<Container, std::void_t<typename Container::key_type>> =
    true;

// The entry a registry keeps for `Class`: the v-table pointer itself, or, under
// indirect_vptr, the address of the class' static_vptr. Either way, comparing
// it before and after says whether the slot was carried over. Absent - a hole
// in the vector, or no key in the map - reads as null.
template<class Registry, class Class>
auto entry_for() {
    using vptr_state =
        typename Registry::template policy<policies::vptr>::state;
    auto& vptrs = detail::get<vptr_state>(Registry::state().policies).vptrs;
    auto type = Registry::rtti::template static_type<Class>();

    if constexpr (is_map<std::decay_t<decltype(vptrs)>>) {
        auto iter = vptrs.find(type);
        typename std::decay_t<decltype(vptrs)>::mapped_type entry = nullptr;

        if (iter != vptrs.end()) {
            entry = iter->second;
        }

        return entry;
    } else {
        auto index = std::size_t(type);

        return index < vptrs.size() ? vptrs[index] : nullptr;
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    dropped_class_does_not_keep_its_vptr, Registry, registries<__COUNTER__>) {
    // Function-local statics: register on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<Animal, Dog, Cat, Registry> BOOST_OPENMETHOD_GENSYM;
    static typename speak<Registry>::template override<speak_animal<Registry>>
        BOOST_OPENMETHOD_GENSYM;

    decltype(entry_for<Registry, Tiger>()) tiger_entry;

    {
        // A test-only spelling: a registrar is documented to be a static
        // object (core.hpp, on `override`; macros.hpp, on
        // BOOST_OPENMETHOD_REGISTER; shared_libraries.adoc). One is used here
        // because a static never dies before the program does, and this test
        // needs the registration to go away between the two initialize()
        // calls - which is what unloading a library does to the classes it
        // brought. Do not copy this into an example.
        //
        // The braces are load-bearing. A registrar links itself into the
        // registry's static_list, whose links carry no initializer
        // (`static_link() = default`, detail/static_list.hpp:25-34) - on
        // purpose: registrars live in static storage, which is zeroed before
        // any dynamic initialization, so a registrar can link itself in
        // whatever order the translation units' constructors run, and the list
        // head cannot be constructed after it and wipe the registrations.
        // That is what the `coverity[uninit] - zero-initialized static
        // storage` note on the push_back in core.hpp is recording. An
        // automatic registrar gets none of that, so it has to be
        // value-initialised, or push_back asserts on the garbage.
        use_classes<Animal, Tiger, Registry> tiger_classes{};

        initialize<Registry>();

        Tiger tiger;
        BOOST_TEST(speak<Registry>::fn(tiger) == "...");

        tiger_entry = entry_for<Registry, Tiger>();
        BOOST_TEST(tiger_entry != nullptr);
    }

    // Tiger is gone; the dispatch data its v-table lived in is freed and
    // replaced by this call.
    initialize<Registry>();

    auto tiger_entry_now = entry_for<Registry, Tiger>();
    BOOST_TEST(tiger_entry_now == nullptr);
    BOOST_TEST(tiger_entry_now != tiger_entry);

    // The classes that are still registered keep working.
    Dog dog;
    Cat cat;
    BOOST_TEST(speak<Registry>::fn(dog) == "...");
    BOOST_TEST(speak<Registry>::fn(cat) == "...");
}
