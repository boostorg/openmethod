// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod/interop/std_weak_ptr.hpp>

#define BOOST_TEST_MODULE weak_virtual_ptr
#include <boost/test/unit_test.hpp>

#include "test_virtual_ptr_value_semantics.hpp"

#include <memory>
#include <set>
#include <string>
#include <type_traits>

// A weak virtual_ptr is not a virtual_ptr at all: neither a smart one in the
// `IsSmartPtr` sense (no `virtual_traits`, no `rebind`) nor a plain one.
static_assert(!IsSmartPtr<std::weak_ptr<Animal>, default_registry>);
static_assert(!is_virtual_ptr<weak_virtual_ptr<Animal>>);

// Moves are noexcept, so containers relocate by moving, not copying.
static_assert(std::is_nothrow_move_constructible_v<weak_virtual_ptr<Animal>>);
static_assert(std::is_nothrow_move_assignable_v<weak_virtual_ptr<Animal>>);

static_assert(std::is_same_v<weak_virtual_ptr<Animal>::element_type, Animal>);
static_assert(std::is_same_v<
              decltype(std::declval<weak_virtual_ptr<Animal>>().lock()),
              shared_virtual_ptr<Animal>>);
static_assert(std::is_same_v<
              decltype(std::declval<weak_virtual_ptr<Animal>>().pointer()),
              const std::weak_ptr<Animal>&>);

// Construction is allowed from shared and weak pointers, virtual or not...
static_assert(std::is_constructible_v<
              weak_virtual_ptr<Animal>, shared_virtual_ptr<Animal>>);
static_assert(
    std::is_constructible_v<weak_virtual_ptr<Animal>, shared_virtual_ptr<Dog>>);
static_assert(
    std::is_constructible_v<weak_virtual_ptr<Animal>, weak_virtual_ptr<Dog>>);
static_assert(
    std::is_constructible_v<weak_virtual_ptr<Animal>, std::shared_ptr<Dog>>);
static_assert(
    std::is_constructible_v<weak_virtual_ptr<Animal>, std::weak_ptr<Dog>>);
static_assert(std::is_constructible_v<
              weak_virtual_ptr<const Animal>, shared_virtual_ptr<Dog>>);

// ...but not from a plain pointer, reference or virtual_ptr, nor from a
// different class or a const object...
static_assert(!std::is_constructible_v<weak_virtual_ptr<Animal>, Animal&>);
static_assert(!std::is_constructible_v<weak_virtual_ptr<Animal>, Animal*>);
static_assert(
    !std::is_constructible_v<weak_virtual_ptr<Animal>, virtual_ptr<Animal>>);
static_assert(
    !std::is_constructible_v<weak_virtual_ptr<Cat>, shared_virtual_ptr<Dog>>);
static_assert(!std::is_constructible_v<
              weak_virtual_ptr<Animal>, shared_virtual_ptr<const Animal>>);
static_assert(
    !std::is_constructible_v<
        weak_virtual_ptr<NonPolymorphic>, std::shared_ptr<NonPolymorphic>>);
static_assert(!std::is_constructible_v<
              weak_virtual_ptr<NonPolymorphic>, std::weak_ptr<NonPolymorphic>>);

// ...and a weak virtual_ptr converts to nothing but another weak virtual_ptr.
static_assert(
    !std::is_constructible_v<virtual_ptr<Animal>, weak_virtual_ptr<Animal>>);
static_assert(
    !std::is_assignable_v<virtual_ptr<Animal>&, weak_virtual_ptr<Animal>>);
static_assert(!std::is_constructible_v<
              shared_virtual_ptr<Animal>, weak_virtual_ptr<Animal>>);
static_assert(!std::is_assignable_v<
              shared_virtual_ptr<Animal>&, weak_virtual_ptr<Animal>>);
static_assert(!std::is_constructible_v<
              std::shared_ptr<Animal>, weak_virtual_ptr<Animal>>);

BOOST_AUTO_TEST_CASE_TEMPLATE(
    weak_virtual_ptr_from_shared_virtual_ptr, Registry, test_policies) {
    init_test<Registry>();

    auto snoopy = std::make_shared<Dog>();
    shared_virtual_ptr<Animal, Registry> shared(snoopy);

    weak_virtual_ptr<Animal, Registry> weak(shared);
    BOOST_TEST(!weak.expired());
    BOOST_TEST(weak.use_count() == 2);
    BOOST_TEST(weak.vptr() == Registry::template static_vptr<Dog>);
    BOOST_TEST(weak.pointer().lock() == snoopy);

    {
        auto locked = weak.lock();
        static_assert(std::is_same_v<
                      decltype(locked), shared_virtual_ptr<Animal, Registry>>);
        BOOST_TEST(locked.get() == snoopy.get());
        BOOST_TEST(locked.vptr() == Registry::template static_vptr<Dog>);
        BOOST_TEST(weak.use_count() == 3);
    }

    BOOST_TEST(weak.use_count() == 2);

    shared = nullptr;
    snoopy.reset();
    BOOST_TEST(weak.expired());
    BOOST_TEST(weak.use_count() == 0);

    auto locked = weak.lock();
    BOOST_TEST(locked.get() == nullptr);
    BOOST_TEST(locked.vptr() == nullptr);

    weak.reset();
    BOOST_TEST(weak.expired());
    BOOST_TEST(weak.vptr() == nullptr);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    weak_virtual_ptr_from_std_pointers, Registry, test_policies) {
    init_test<Registry>();

    auto felix = std::make_shared<Cat>();

    {
        weak_virtual_ptr<Animal, Registry> weak(felix);
        BOOST_TEST(weak.vptr() == Registry::template static_vptr<Cat>);
        BOOST_TEST(weak.lock().get() == felix.get());
    }

    {
        std::weak_ptr<Cat> std_weak = felix;
        weak_virtual_ptr<Animal, Registry> weak(std_weak);
        BOOST_TEST(weak.vptr() == Registry::template static_vptr<Cat>);
        BOOST_TEST(weak.lock().get() == felix.get());
    }

    {
        // an expired std::weak_ptr yields an expired weak virtual_ptr
        std::weak_ptr<Cat> std_weak;
        {
            auto dead = std::make_shared<Cat>();
            std_weak = dead;
        }

        weak_virtual_ptr<Animal, Registry> weak(std_weak);
        BOOST_TEST(weak.expired());
        BOOST_TEST(weak.vptr() == nullptr);
        BOOST_TEST(weak.lock().get() == nullptr);
        BOOST_TEST(weak.use_count() == 0);
    }

    {
        // an expired source of the same class keeps its control block, so it
        // keeps its owner identity. Across a class conversion that is up to
        // the standard library: libstdc++ shares ownership, as
        // [util.smartptr.weak.const] requires of a source that is expired but
        // not empty; libc++ locks first, and an expired source then yields an
        // empty weak pointer.
        std::weak_ptr<Animal> std_weak;
        {
            auto dead = std::make_shared<Animal>();
            std_weak = dead;
        }

        weak_virtual_ptr<Animal, Registry> weak(std_weak);
        BOOST_TEST(weak.expired());
        BOOST_TEST(weak.vptr() == nullptr);
        BOOST_TEST(!weak.pointer().owner_before(std_weak));
        BOOST_TEST(!std_weak.owner_before(weak.pointer()));
    }

    {
        // an empty std::shared_ptr yields an empty weak virtual_ptr
        weak_virtual_ptr<Animal, Registry> weak{std::shared_ptr<Dog>()};
        BOOST_TEST(weak.expired());
        BOOST_TEST(weak.vptr() == nullptr);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    weak_virtual_ptr_default_and_nullptr, Registry, test_policies) {
    init_test<Registry>();

    {
        weak_virtual_ptr<Animal, Registry> weak;
        BOOST_TEST(weak.expired());
        BOOST_TEST(weak.use_count() == 0);
        BOOST_TEST(weak.vptr() == nullptr);
        BOOST_TEST(weak.lock().get() == nullptr);
    }

    {
        weak_virtual_ptr<Animal, Registry> weak(nullptr);
        BOOST_TEST(weak.expired());
        BOOST_TEST(weak.vptr() == nullptr);
    }

    {
        auto snoopy = std::make_shared<Dog>();
        weak_virtual_ptr<Animal, Registry> weak(snoopy);
        BOOST_TEST(!weak.expired());

        weak = nullptr;
        BOOST_TEST(weak.expired());
        BOOST_TEST(weak.vptr() == nullptr);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    weak_virtual_ptr_assign, Registry, test_policies) {
    init_test<Registry>();

    auto snoopy = std::make_shared<Dog>();
    auto felix = std::make_shared<Cat>();
    weak_virtual_ptr<Animal, Registry> weak;

    weak = shared_virtual_ptr<Animal, Registry>(snoopy);
    BOOST_TEST(weak.lock().get() == snoopy.get());
    BOOST_TEST(weak.vptr() == Registry::template static_vptr<Dog>);

    weak = shared_virtual_ptr<Cat, Registry>(felix);
    BOOST_TEST(weak.lock().get() == felix.get());
    BOOST_TEST(weak.vptr() == Registry::template static_vptr<Cat>);

    weak = snoopy;
    BOOST_TEST(weak.lock().get() == snoopy.get());
    BOOST_TEST(weak.vptr() == Registry::template static_vptr<Dog>);

    weak = std::weak_ptr<Cat>(felix);
    BOOST_TEST(weak.lock().get() == felix.get());
    BOOST_TEST(weak.vptr() == Registry::template static_vptr<Cat>);

    weak_virtual_ptr<Dog, Registry> weak_dog(snoopy);
    weak = weak_dog;
    BOOST_TEST(weak.lock().get() == snoopy.get());
    BOOST_TEST(weak.vptr() == Registry::template static_vptr<Dog>);
    BOOST_TEST(!weak_dog.expired());

    weak = *&weak; // self-assignment
    BOOST_TEST(weak.lock().get() == snoopy.get());
    BOOST_TEST(weak.vptr() == Registry::template static_vptr<Dog>);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    weak_virtual_ptr_copy_move, Registry, test_policies) {
    init_test<Registry>();

    auto snoopy = std::make_shared<Dog>();
    weak_virtual_ptr<Dog, Registry> weak_dog(snoopy);

    {
        weak_virtual_ptr<Dog, Registry> copy(weak_dog);
        BOOST_TEST(copy.lock().get() == snoopy.get());
        BOOST_TEST(copy.vptr() == Registry::template static_vptr<Dog>);
        BOOST_TEST(weak_dog.lock().get() == snoopy.get());
    }

    {
        // upcast, copying
        weak_virtual_ptr<Animal, Registry> base(weak_dog);
        BOOST_TEST(base.lock().get() == snoopy.get());
        BOOST_TEST(base.vptr() == Registry::template static_vptr<Dog>);
        BOOST_TEST(weak_dog.lock().get() == snoopy.get());
    }

    {
        weak_virtual_ptr<Dog, Registry> source(snoopy);
        weak_virtual_ptr<Dog, Registry> moved(std::move(source));
        BOOST_TEST(moved.lock().get() == snoopy.get());
        BOOST_TEST(moved.vptr() == Registry::template static_vptr<Dog>);
        BOOST_TEST(source.expired());
        BOOST_TEST(source.vptr() == nullptr);
    }

    {
        // upcast, moving
        weak_virtual_ptr<Dog, Registry> source(snoopy);
        weak_virtual_ptr<Animal, Registry> moved(std::move(source));
        BOOST_TEST(moved.lock().get() == snoopy.get());
        BOOST_TEST(moved.vptr() == Registry::template static_vptr<Dog>);
        BOOST_TEST(source.expired());
        BOOST_TEST(source.vptr() == nullptr);
    }

    {
        weak_virtual_ptr<Dog, Registry> source(snoopy);
        weak_virtual_ptr<Animal, Registry> moved;
        moved = std::move(source);
        BOOST_TEST(moved.lock().get() == snoopy.get());
        BOOST_TEST(moved.vptr() == Registry::template static_vptr<Dog>);
        BOOST_TEST(source.expired());
        BOOST_TEST(source.vptr() == nullptr);
    }

    {
        auto felix = std::make_shared<Cat>();
        weak_virtual_ptr<Animal, Registry> weak_cat(felix);
        weak_virtual_ptr<Animal, Registry> weak_animal(weak_dog);
        weak_cat.swap(weak_animal);
        BOOST_TEST(weak_cat.lock().get() == snoopy.get());
        BOOST_TEST(weak_cat.vptr() == Registry::template static_vptr<Dog>);
        BOOST_TEST(weak_animal.lock().get() == felix.get());
        BOOST_TEST(weak_animal.vptr() == Registry::template static_vptr<Cat>);
    }
}

// `std::owner_less<void>` is generic only in libstdc++; MSVC's and libc++'s
// have overloads for `std::shared_ptr` and `std::weak_ptr` alone, so a
// container keyed on owner identity carries its own comparator.
struct owner_less {
    template<class Left, class Right>
    auto operator()(const Left& left, const Right& right) const -> bool {
        return left.owner_before(right);
    }
};

BOOST_AUTO_TEST_CASE_TEMPLATE(
    weak_virtual_ptr_owner_before, Registry, test_policies) {
    init_test<Registry>();

    auto snoopy = std::make_shared<Dog>();
    auto felix = std::make_shared<Cat>();
    shared_virtual_ptr<Animal, Registry> shared_dog(snoopy);
    weak_virtual_ptr<Animal, Registry> weak_dog(shared_dog);
    weak_virtual_ptr<Dog, Registry> weak_dog_too(snoopy);
    weak_virtual_ptr<Animal, Registry> weak_cat(felix);

    // same owner, against a shared and a weak virtual_ptr, to another class
    BOOST_TEST(!weak_dog.owner_before(shared_dog));
    BOOST_TEST(!weak_dog.owner_before(weak_dog_too));
    BOOST_TEST(!weak_dog_too.owner_before(weak_dog));

    // different owners: a strict weak ordering, the same as std::weak_ptr's
    BOOST_TEST(
        weak_dog.owner_before(weak_cat) != weak_cat.owner_before(weak_dog));
    BOOST_TEST(
        weak_dog.owner_before(weak_cat) ==
        std::weak_ptr<Animal>(snoopy).owner_before(felix));

    std::set<weak_virtual_ptr<Animal, Registry>, owner_less> observers;
    observers.insert(weak_dog);
    observers.insert(weak_cat);
    observers.insert(weak_virtual_ptr<Animal, Registry>(snoopy)); // same owner
    BOOST_TEST(observers.size() == 2u);
    BOOST_TEST(observers.count(weak_dog) == 1u);
    BOOST_TEST(observers.count(weak_cat) == 1u);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    weak_virtual_ptr_non_polymorphic, Registry, test_policies) {
    // The v-table pointer is copied from the shared_virtual_ptr, so the class
    // need not be polymorphic; only the vptr lookup would require that.
    //
    // Function-local static: registers on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<NonPolymorphic, Registry> BOOST_OPENMETHOD_GENSYM;
    init_test<Registry>();

    auto shared = make_shared_virtual<NonPolymorphic, Registry>();
    weak_virtual_ptr<NonPolymorphic, Registry> weak(shared);
    BOOST_TEST(weak.vptr() == Registry::template static_vptr<NonPolymorphic>);
    BOOST_TEST(weak.lock().get() == shared.get());
    BOOST_TEST(
        weak.lock().vptr() == Registry::template static_vptr<NonPolymorphic>);
}

struct BOOST_OPENMETHOD_ID(poke);

// Namespace-scope templates: gcc before 13 does not accept the static member
// functions of a local class as template arguments (no linkage).
template<class Registry>
auto poke_dog(shared_virtual_ptr<Dog, Registry>) -> std::string {
    return "bark";
}

template<class Registry>
auto poke_cat(shared_virtual_ptr<Cat, Registry>) -> std::string {
    return "hiss";
}

struct BOOST_OPENMETHOD_ID(observe);

// A weak_virtual_ptr is not a virtual_ptr, so it can be an ordinary, non-virtual
// parameter of a method: the object it tracks is not dispatched on.
template<class Registry>
auto observe_dog(
    virtual_ptr<Dog, Registry>, weak_virtual_ptr<Animal, Registry> other)
    -> std::string {
    return other.expired() ? "dog sees nobody" : "dog sees somebody";
}

template<class Registry>
auto observe_cat(
    virtual_ptr<Cat, Registry>, weak_virtual_ptr<Animal, Registry> other)
    -> std::string {
    return other.expired() ? "cat sees nobody" : "cat sees somebody";
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    weak_virtual_ptr_non_virtual_parameter, Registry, test_policies) {
    using observe = method<
        BOOST_OPENMETHOD_ID(observe),
        auto(virtual_ptr<Animal, Registry>, weak_virtual_ptr<Animal, Registry>)
            ->std::string,
        Registry>;

    // Function-local static: registers on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static typename observe::template override<
        observe_dog<Registry>, observe_cat<Registry>>
        BOOST_OPENMETHOD_GENSYM;

    init_test<Registry>();

    auto snoopy = std::make_shared<Dog>();
    auto felix = std::make_shared<Cat>();
    virtual_ptr<Animal, Registry> dog(*snoopy);
    virtual_ptr<Animal, Registry> cat(*felix);
    weak_virtual_ptr<Animal, Registry> weak_dog(snoopy);
    weak_virtual_ptr<Animal, Registry> weak_cat(felix);

    BOOST_TEST(observe::fn(dog, weak_cat) == "dog sees somebody");
    BOOST_TEST(observe::fn(cat, weak_dog) == "cat sees somebody");

    felix.reset();
    BOOST_TEST(observe::fn(dog, weak_cat) == "dog sees nobody");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(
    weak_virtual_ptr_dispatch, Registry, test_policies) {
    using poke = method<
        BOOST_OPENMETHOD_ID(poke),
        auto(shared_virtual_ptr<Animal, Registry>)->std::string, Registry>;

    // Function-local static: registers on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static
        typename poke::template override<poke_dog<Registry>, poke_cat<Registry>>
            BOOST_OPENMETHOD_GENSYM;

    init_test<Registry>();

    auto snoopy = std::make_shared<Dog>();
    auto felix = std::make_shared<Cat>();
    weak_virtual_ptr<Animal, Registry> weak_dog(snoopy);
    weak_virtual_ptr<Animal, Registry> weak_cat(felix);

    // lock, then dispatch
    BOOST_TEST(poke::fn(weak_dog.lock()) == "bark");
    BOOST_TEST(poke::fn(weak_cat.lock()) == "hiss");
}
