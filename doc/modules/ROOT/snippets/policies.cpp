// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/policies/minimal_cover_hash.hpp>
#include <boost/openmethod/policies/minimal_perfect_hash.hpp>
#include <boost/openmethod/policies/throw_error_handler.hpp>
#include <boost/openmethod/policies/two_level_hash.hpp>
#include <boost/openmethod/policies/vptr_map.hpp>

#include <stdexcept>
#include <variant>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include "capture.hpp"

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};
struct Cat : Animal {};
struct Dog : Animal {};

// The registries below each get their own copy of the classes and of `trick`.
// Only `Dog` has an overrider, so calling `trick` on a `Cat` reaches the
// registry's error handler.

namespace std_rtti_demo {

// tag::std_rtti[]
struct dynamic_registry :
    registry<
        policies::std_rtti, policies::fast_perfect_hash,
        policies::vptr_vector> {};
// end::std_rtti[]

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog, dynamic_registry);

BOOST_OPENMETHOD(
    trick, (virtual_ptr<Animal, dynamic_registry>), std::string,
    dynamic_registry);

BOOST_OPENMETHOD_OVERRIDE(
    trick, (virtual_ptr<Dog, dynamic_registry>), std::string) {
    return "spin";
}

} // namespace std_rtti_demo

namespace vptr_vector_demo {

// tag::vptr_vector[]
// `fast_perfect_hash` turns the type ids into small indices; without it the
// vector is indexed by the type id itself, which `std_rtti` makes a pointer
struct vector_registry :
    registry<
        policies::std_rtti, policies::fast_perfect_hash,
        policies::vptr_vector> {};
// end::vptr_vector[]

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog, vector_registry);

BOOST_OPENMETHOD(
    trick, (virtual_ptr<Animal, vector_registry>), std::string,
    vector_registry);

BOOST_OPENMETHOD_OVERRIDE(
    trick, (virtual_ptr<Dog, vector_registry>), std::string) {
    return "spin";
}

} // namespace vptr_vector_demo

namespace vptr_map_demo {

// tag::vptr_map[]
struct map_registry : registry<policies::std_rtti, policies::vptr_map<>> {};
// end::vptr_map[]

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog, map_registry);

BOOST_OPENMETHOD(
    trick, (virtual_ptr<Animal, map_registry>), std::string, map_registry);

BOOST_OPENMETHOD_OVERRIDE(
    trick, (virtual_ptr<Dog, map_registry>), std::string) {
    return "spin";
}

} // namespace vptr_map_demo

namespace fast_perfect_hash_demo {

// tag::fast_perfect_hash[]
// `vptr_vector` indexes by the type id unless a `type_hash` policy maps it to
// a small integer first. With `std_rtti`, where a type id is a pointer, that
// makes the difference between a vector of a few entries and one that cannot
// be allocated at all.
struct hashed_registry :
    registry<
        policies::std_rtti, policies::fast_perfect_hash,
        policies::vptr_vector> {};
// end::fast_perfect_hash[]

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog, hashed_registry);

BOOST_OPENMETHOD(
    trick, (virtual_ptr<Animal, hashed_registry>), std::string,
    hashed_registry);

BOOST_OPENMETHOD_OVERRIDE(
    trick, (virtual_ptr<Dog, hashed_registry>), std::string) {
    return "spin";
}

} // namespace fast_perfect_hash_demo

namespace minimal_perfect_hash_demo {

// tag::minimal_perfect_hash[]
// One slot per type id, whatever the addresses are. Swapping the hash is all it
// takes: `with` replaces the policy of the same category, in place, so
// `vptr_vector` still comes after it.
struct compact_registry :
    default_registry::with<policies::minimal_perfect_hash<>> {};
// end::minimal_perfect_hash[]

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog, compact_registry);

BOOST_OPENMETHOD(
    trick, (virtual_ptr<Animal, compact_registry>), std::string,
    compact_registry);

BOOST_OPENMETHOD_OVERRIDE(
    trick, (virtual_ptr<Dog, compact_registry>), std::string) {
    return "spin";
}

} // namespace minimal_perfect_hash_demo

namespace two_level_hash_demo {

// tag::two_level_hash[]
struct two_level_registry :
    default_registry::with<policies::two_level_hash<>> {};
// end::two_level_hash[]

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog, two_level_registry);

BOOST_OPENMETHOD(
    trick, (virtual_ptr<Animal, two_level_registry>), std::string,
    two_level_registry);

BOOST_OPENMETHOD_OVERRIDE(
    trick, (virtual_ptr<Dog, two_level_registry>), std::string) {
    return "spin";
}

} // namespace two_level_hash_demo

#if BOOST_OPENMETHOD_HAS_PEXT

namespace minimal_cover_hash_demo {

// tag::minimal_cover_hash[]
// Needs BMI2, for every translation unit of the program - hence the guard.
#if BOOST_OPENMETHOD_HAS_PEXT
struct cover_registry :
    default_registry::with<policies::minimal_cover_hash<>> {};
#endif
// end::minimal_cover_hash[]

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog, cover_registry);

BOOST_OPENMETHOD(
    trick, (virtual_ptr<Animal, cover_registry>), std::string, cover_registry);

BOOST_OPENMETHOD_OVERRIDE(
    trick, (virtual_ptr<Dog, cover_registry>), std::string) {
    return "spin";
}

} // namespace minimal_cover_hash_demo

#endif

namespace stderr_output_demo {

// tag::stderr_output[]
struct noisy_registry :
    registry<
        policies::std_rtti, policies::fast_perfect_hash, policies::vptr_vector,
        policies::default_error_handler, policies::stderr_output> {};
// end::stderr_output[]

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog, noisy_registry);

BOOST_OPENMETHOD(
    trick, (virtual_ptr<Animal, noisy_registry>), std::string, noisy_registry);

BOOST_OPENMETHOD_OVERRIDE(
    trick, (virtual_ptr<Dog, noisy_registry>), std::string) {
    return "spin";
}

} // namespace stderr_output_demo

namespace default_error_handler_demo {

// tag::default_error_handler_registry[]
struct handled_registry :
    registry<
        policies::std_rtti, policies::fast_perfect_hash, policies::vptr_vector,
        policies::default_error_handler, policies::stderr_output> {};
// end::default_error_handler_registry[]

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog, handled_registry);

BOOST_OPENMETHOD(
    trick, (virtual_ptr<Animal, handled_registry>), std::string,
    handled_registry);

BOOST_OPENMETHOD_OVERRIDE(
    trick, (virtual_ptr<Dog, handled_registry>), std::string) {
    return "spin";
}

} // namespace default_error_handler_demo

namespace throw_error_handler_demo {

// tag::throw_error_handler_registry[]
struct throwing_registry :
    registry<
        policies::std_rtti, policies::fast_perfect_hash, policies::vptr_vector,
        policies::throw_error_handler> {};
// end::throw_error_handler_registry[]

BOOST_OPENMETHOD_CLASSES(Animal, Cat, Dog, throwing_registry);

BOOST_OPENMETHOD(
    trick, (virtual_ptr<Animal, throwing_registry>), std::string,
    throwing_registry);

BOOST_OPENMETHOD_OVERRIDE(
    trick, (virtual_ptr<Dog, throwing_registry>), std::string) {
    return "spin";
}

} // namespace throw_error_handler_demo

BOOST_AUTO_TEST_CASE(rtti_and_storage) {
    {
        using namespace std_rtti_demo;
        initialize<dynamic_registry>();
        capture_cout cout;

        // tag::std_rtti_dispatch[]
        Dog snoopy;
        Animal& animal = snoopy;

        std::cout << trick(virtual_ptr<Animal, dynamic_registry>(animal))
                  << "\n"; // spin
        // end::std_rtti_dispatch[]

        BOOST_TEST(cout.str() == "spin\n");
    }

    {
        using namespace vptr_vector_demo;
        initialize<vector_registry>();

        Dog snoopy;
        BOOST_TEST(
            trick(virtual_ptr<Animal, vector_registry>(snoopy)) == "spin");
    }

    {
        using namespace vptr_map_demo;
        initialize<map_registry>();

        Dog snoopy;
        BOOST_TEST(trick(virtual_ptr<Animal, map_registry>(snoopy)) == "spin");
    }

    {
        using namespace fast_perfect_hash_demo;
        initialize<hashed_registry>();

        Dog snoopy;
        BOOST_TEST(
            trick(virtual_ptr<Animal, hashed_registry>(snoopy)) == "spin");
    }

    {
        using namespace minimal_perfect_hash_demo;
        initialize<compact_registry>();

        Dog snoopy;
        BOOST_TEST(
            trick(virtual_ptr<Animal, compact_registry>(snoopy)) == "spin");
    }

    {
        using namespace two_level_hash_demo;
        initialize<two_level_registry>();

        Dog snoopy;
        BOOST_TEST(
            trick(virtual_ptr<Animal, two_level_registry>(snoopy)) == "spin");
    }

#if BOOST_OPENMETHOD_HAS_PEXT
    {
        using namespace minimal_cover_hash_demo;
        initialize<cover_registry>();

        Dog snoopy;
        BOOST_TEST(
            trick(virtual_ptr<Animal, cover_registry>(snoopy)) == "spin");
    }
#endif

    {
        using namespace stderr_output_demo;
        initialize<noisy_registry>();

        Dog snoopy;
        BOOST_TEST(
            trick(virtual_ptr<Animal, noisy_registry>(snoopy)) == "spin");
    }
}

BOOST_AUTO_TEST_CASE(error_handlers) {
    {
        using namespace default_error_handler_demo;
        initialize<handled_registry>();

        capture_cerr cerr;

        // tag::default_error_handler_set[]
        handled_registry::error_handler::set([](const auto& error) {
            if (std::holds_alternative<no_overrider>(error)) {
                throw std::runtime_error("not implemented");
            }
        });

        Cat felix;

        try {
            trick(virtual_ptr<Animal, handled_registry>(felix));
        } catch (const std::runtime_error& error) {
            std::cerr << error.what() << "\n"; // not implemented
        }
        // end::default_error_handler_set[]

        BOOST_TEST(cerr.str() == "not implemented\n");
    }

    {
        using namespace throw_error_handler_demo;
        initialize<throwing_registry>();

        capture_cerr cerr;

        // tag::throw_error_handler_catch[]
        Cat felix;

        try {
            trick(virtual_ptr<Animal, throwing_registry>(felix));
        } catch (const no_overrider&) {
            std::cerr << "no overrider for Cat\n";
        }
        // end::throw_error_handler_catch[]

        BOOST_TEST(cerr.str() == "no overrider for Cat\n");
    }
}
