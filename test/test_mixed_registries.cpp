// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Methods whose virtual parameters belong to different registries, each with
// its own rtti policy. `zoo_registry` and `garage_registry` identify classes
// by a number stored in the object, and give the same numbers to unrelated
// classes: a registry that looked up another's type_ids in its own tables
// would find the wrong class. `space_registry` uses standard RTTI.

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>
#include <boost/openmethod/initialize.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <type_traits>

#define BOOST_TEST_MODULE mixed_registries
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

namespace {

auto next_non_polymorphic_id() -> std::size_t {
    static std::size_t counter = 0;

    return (std::size_t(1) << (sizeof(std::size_t) * 8 - 1)) | ++counter;
}

template<typename T>
auto non_polymorphic_static_type() -> std::size_t {
    static std::size_t value = next_non_polymorphic_id();

    return value;
}

// Classes derived from `Root` carry their type number in `type`.
template<class Root>
struct number_rtti : policies::rtti {
    template<class Registry>
    struct fn : defaults {
        template<class T>
        static constexpr bool is_polymorphic = std::is_base_of_v<Root, T>;

        template<typename T>
        static auto static_type() -> type_id {
            if constexpr (is_polymorphic<T>) {
                return type_id(T::static_type);
            } else {
                return type_id(non_polymorphic_static_type<T>());
            }
        }

        template<typename T>
        static auto dynamic_type(const T& obj) -> type_id {
            if constexpr (is_polymorphic<T>) {
                return type_id(obj.type);
            } else {
                return type_id(non_polymorphic_static_type<T>());
            }
        }

        template<class Stream>
        static void type_name(type_id type, Stream& stream) {
            auto index = reinterpret_cast<std::size_t>(type);
            stream << (index >= 1 && index <= 3 ? Root::names[index - 1] : "?");
        }

        static auto type_index(type_id type) -> type_id {
            return type;
        }
    };
};

struct Animal {
    static constexpr std::size_t static_type = 1;
    static constexpr const char* names[] = {"Animal", "Dog", "Cat"};
    std::size_t type;

    explicit Animal(std::size_t type = static_type) : type(type) {
    }
};

struct Dog : Animal {
    static constexpr std::size_t static_type = 2;

    Dog() : Animal(static_type) {
    }
};

struct Cat : Animal {
    static constexpr std::size_t static_type = 3;

    Cat() : Animal(static_type) {
    }
};

// Same type numbers as the animals.
struct Vehicle {
    static constexpr std::size_t static_type = 1;
    static constexpr const char* names[] = {"Vehicle", "Car", "Truck"};
    std::size_t type;

    explicit Vehicle(std::size_t type = static_type) : type(type) {
    }
};

struct Car : Vehicle {
    static constexpr std::size_t static_type = 2;

    Car() : Vehicle(static_type) {
    }
};

struct Truck : Vehicle {
    static constexpr std::size_t static_type = 3;

    Truck() : Vehicle(static_type) {
    }
};

struct Planet {
    virtual ~Planet() = default;
};

struct Earth : Planet {};
struct Mars : Planet {};

struct zoo_registry :
    default_registry::with<number_rtti<Animal>>::without<policies::type_hash> {
};

// Checked, because it holds methods with parameters in `zoo_registry`.
struct garage_registry :
    default_registry::with<number_rtti<Vehicle>>::without<policies::type_hash>::
        with<policies::runtime_checks, policies::throw_error_handler> {};

struct space_registry :
    default_registry::with<policies::throw_error_handler> {};

// Holds methods only; every parameter is foreign.
struct arbiter_registry :
    default_registry::with<
        policies::runtime_checks, policies::throw_error_handler> {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat, zoo_registry);
BOOST_OPENMETHOD_CLASSES(Vehicle, Car, Truck, garage_registry);
BOOST_OPENMETHOD_CLASSES(Planet, Earth, Mars, space_registry);

// -----------------------------------------------------------------------------
// A method of the registry of its parameter, sharing the v-tables with the
// foreign parameters below.

BOOST_OPENMETHOD(kind, (virtual_<const Animal&>), std::string, zoo_registry);

BOOST_OPENMETHOD_OVERRIDE(kind, (const Animal&), std::string) {
    return "animal";
}

BOOST_OPENMETHOD_OVERRIDE(kind, (const Dog&), std::string) {
    return "dog";
}

BOOST_OPENMETHOD_OVERRIDE(kind, (const Cat&), std::string) {
    return "cat";
}

// -----------------------------------------------------------------------------
// Foreign first parameter: its entry, a pointer into the method's dispatch
// table, is written in the v-tables of `zoo_registry`.

BOOST_OPENMETHOD(
    run_over, (virtual_<const Animal&, zoo_registry>, virtual_<const Vehicle&>),
    std::string, garage_registry);

BOOST_OPENMETHOD_OVERRIDE(
    run_over, (const Animal&, const Vehicle&), std::string) {
    return "animal, vehicle";
}

BOOST_OPENMETHOD_OVERRIDE(run_over, (const Dog&, const Car&), std::string) {
    return "dog, car";
}

BOOST_OPENMETHOD_OVERRIDE(run_over, (const Cat&, const Truck&), std::string) {
    return "cat, truck";
}

// Two equally good overriders for (Dog, Truck).
BOOST_OPENMETHOD_OVERRIDE(
    run_over, (const Dog& dog, const Vehicle& vehicle), std::string) {
    return "dog, vehicle -> " + next(dog, vehicle);
}

BOOST_OPENMETHOD_OVERRIDE(
    run_over, (const Animal&, const Truck&), std::string) {
    return "animal, truck";
}

// -----------------------------------------------------------------------------
// Foreign second parameter.

BOOST_OPENMETHOD(
    carry, (virtual_<const Vehicle&>, virtual_<const Animal&, zoo_registry>),
    std::string, garage_registry);

BOOST_OPENMETHOD_OVERRIDE(carry, (const Car&, const Dog&), std::string) {
    return "car, dog";
}

BOOST_OPENMETHOD_OVERRIDE(carry, (const Truck&, const Animal&), std::string) {
    return "truck, animal";
}

// -----------------------------------------------------------------------------
// A uni-method on a foreign parameter, in a registry that has no classes.

BOOST_OPENMETHOD(
    horn, (virtual_<const Vehicle&, garage_registry>), std::string,
    arbiter_registry);

BOOST_OPENMETHOD_OVERRIDE(horn, (const Car&), std::string) {
    return "beep";
}

BOOST_OPENMETHOD_OVERRIDE(horn, (const Truck&), std::string) {
    return "honk";
}

// -----------------------------------------------------------------------------
// Three registries, three rtti policies, and the `virtual_ptr` shapes.

BOOST_OPENMETHOD(
    land,
    (virtual_ptr<const Animal, zoo_registry>,
        const virtual_ptr<const Vehicle, garage_registry>&,
        virtual_<const Planet&, space_registry>),
    std::string, arbiter_registry);

BOOST_OPENMETHOD_OVERRIDE(
    land,
    (virtual_ptr<const Animal, zoo_registry>,
        const virtual_ptr<const Vehicle, garage_registry>&, const Planet&),
    std::string) {
    return "animal, vehicle, planet";
}

BOOST_OPENMETHOD_OVERRIDE(
    land,
    (virtual_ptr<const Dog, zoo_registry>,
        const virtual_ptr<const Car, garage_registry>&, const Earth&),
    std::string) {
    return "dog, car, earth";
}

BOOST_OPENMETHOD_OVERRIDE(
    land,
    (virtual_ptr<const Cat, zoo_registry>,
        const virtual_ptr<const Truck, garage_registry>&, const Mars&),
    std::string) {
    return "cat, truck, mars";
}

template<class Error, class Fn>
auto throws(Fn fn) -> bool {
    try {
        fn();
    } catch (const Error&) {
        return true;
    }

    return false;
}

void initialize_all() {
    initialize<zoo_registry>(trace::from_env());
    initialize<space_registry>(trace::from_env());
    initialize<garage_registry>(trace::from_env());
    initialize<arbiter_registry>(trace::from_env());
}

} // namespace

BOOST_AUTO_TEST_CASE(parameter_registry_first) {
    // `garage_registry` holds methods with a parameter in `zoo_registry`,
    // which has not been initialized yet.
    BOOST_TEST(throws<parameter_registry_not_initialized>([] {
        initialize<garage_registry>();
    }));

    initialize_all();
}

BOOST_AUTO_TEST_CASE(dispatch) {
    initialize_all();

    Animal animal;
    Dog dog;
    Cat cat;
    Vehicle vehicle;
    Car car;
    Truck truck;
    Earth earth;
    Mars mars;

    BOOST_TEST(kind(animal) == "animal");
    BOOST_TEST(kind(dog) == "dog");
    BOOST_TEST(kind(cat) == "cat");

    BOOST_TEST(run_over(animal, vehicle) == "animal, vehicle");
    BOOST_TEST(run_over(dog, car) == "dog, car");
    BOOST_TEST(run_over(cat, truck) == "cat, truck");
    BOOST_TEST(run_over(cat, car) == "animal, vehicle");
    BOOST_TEST(run_over(dog, vehicle) == "dog, vehicle -> animal, vehicle");
    BOOST_TEST(run_over(animal, truck) == "animal, truck");
    BOOST_TEST(throws<ambiguous_call>([&] { run_over(dog, truck); }));

    BOOST_TEST(carry(car, dog) == "car, dog");
    BOOST_TEST(carry(truck, cat) == "truck, animal");
    BOOST_TEST(throws<no_overrider>([&] { carry(car, cat); }));
    BOOST_TEST(throws<no_overrider>([&] { carry(vehicle, dog); }));

    BOOST_TEST(horn(car) == "beep");
    BOOST_TEST(horn(truck) == "honk");
    BOOST_TEST(throws<no_overrider>([&] { horn(vehicle); }));

    BOOST_TEST(
        land(
            virtual_ptr<const Animal, zoo_registry>(dog),
            virtual_ptr<const Vehicle, garage_registry>(car),
            earth) == "dog, car, earth");
    BOOST_TEST(
        land(
            virtual_ptr<const Animal, zoo_registry>(cat),
            virtual_ptr<const Vehicle, garage_registry>(truck),
            mars) == "cat, truck, mars");
    BOOST_TEST(
        land(
            virtual_ptr<const Animal, zoo_registry>(dog),
            virtual_ptr<const Vehicle, garage_registry>(car),
            mars) == "animal, vehicle, planet");
}

BOOST_AUTO_TEST_CASE(parameter_registry_initialized_again) {
    initialize_all();

    Dog dog;
    Car car;

    BOOST_TEST(run_over(dog, car) == "dog, car");

    // Clears the entries of `garage_registry`'s methods in the v-tables of
    // `zoo_registry`.
    initialize<zoo_registry>();
    BOOST_TEST(kind(dog) == "dog");
    BOOST_TEST(throws<parameter_registry_not_initialized>([&] {
        run_over(dog, car);
    }));
    BOOST_TEST(
        throws<parameter_registry_not_initialized>([&] { carry(car, dog); }));

    // Methods on parameters of other registries are not affected.
    BOOST_TEST(horn(car) == "beep");

    initialize<garage_registry>();
    BOOST_TEST(run_over(dog, car) == "dog, car");
    BOOST_TEST(carry(car, dog) == "car, dog");
}

// A program whose modules share a registry may hold several copies of a
// method, one per module, each registering a record of its foreign parameters.
// Fake a second copy of `carry`: its parameter must share the slot of the
// first, rather than take one of its own.
BOOST_AUTO_TEST_CASE(copies_share_the_slot) {
    using carry_method = BOOST_OPENMETHOD_TYPE(
        carry,
        (virtual_<const Vehicle&>, virtual_<const Animal&, zoo_registry>),
        std::string, garage_registry);
    auto& real = carry_method::fn;
    BOOST_TEST_REQUIRE(real.foreign_end - real.foreign_begin == 1);
    auto& real_param = *real.foreign_begin;

    std::size_t slots_strides[3] = {};
    detail::method_info copy{};
    copy.vp_begin = real.vp_begin;
    copy.vp_end = real.vp_end;
    copy.not_implemented = real.not_implemented;
    copy.ambiguous = real.ambiguous;
    copy.method_type_id = real.method_type_id;
    copy.return_type_id = real.return_type_id;
    copy.slots_strides_ptr = slots_strides;

    detail::foreign_parameter_info param{};
    param.method = &copy;
    param.param = real_param.param;
    param.host_generation = real_param.host_generation;
    param.method_state = real_param.method_state;
    param.same_method = real_param.same_method;
    param.method_type = real_param.method_type;
    param.resolve_vp = real_param.resolve_vp;
    copy.foreign_begin = &param;
    copy.foreign_end = &param + 1;

    garage_registry::state().methods.push_back(copy);
    zoo_registry::state().foreign_parameters.push_back(param);

    initialize_all();

    BOOST_TEST(param.slot == real_param.slot);
    BOOST_TEST_REQUIRE(param.cone.size() == real_param.cone.size());
    BOOST_TEST(param.cone[0].entry == real_param.cone[0].entry);
    BOOST_TEST(
        std::equal(slots_strides, slots_strides + 3, real.slots_strides_ptr));

    Car car;
    Dog dog;
    BOOST_TEST(carry(car, dog) == "car, dog");

    zoo_registry::state().foreign_parameters.remove(param);
    garage_registry::state().methods.remove(copy);
    initialize_all();
}
