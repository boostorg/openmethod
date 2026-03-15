// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define LIBRARY_NAME "boost_openmethod-shared"

// dynamic_main.cpp

#include "animals.hpp"

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <iostream>
#include <memory>

using namespace boost::openmethod;

#ifdef _WIN32

static_assert(!std::is_same_v<
              BOOST_OPENMETHOD_TYPE(
                  meet, (virtual_ptr<Animal>, virtual_ptr<Animal>),
                  std::string)::declspec,
              void>);

static_assert(std::is_same_v<
              BOOST_OPENMETHOD_TYPE(
                  meet, (virtual_ptr<Animal>, virtual_ptr<Animal>),
                  std::string)::declspec,
              dllexport>);

static_assert(!std::is_same_v<default_registry::declspec, void>);
static_assert(std::is_same_v<default_registry::declspec, dllexport>);

#endif

// tag::before[]
// dynamic_main.cpp

BOOST_OPENMETHOD_CLASSES(Herbivore, Cow, Carnivore, Wolf);

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Animal>, virtual_ptr<Animal>), std::string) {
    return "greet";
}

// tag::load[]
int main() {
    // end::load[]
    // end::unload[]

    try {
        std::cout << "Before loading the shared library.\n";

        boost::openmethod::initialize(trace::from_env());
        BOOST_ASSERT(default_registry::static_vptr<Carnivore> != nullptr);

        std::cout << "cow meets wolf -> "
                  << meet(*std::make_unique<Cow>(), *std::make_unique<Wolf>())
                  << "\n"; // greet
        std::cout << "wolf meets cow -> "
                  << meet(*std::make_unique<Wolf>(), *std::make_unique<Cow>())
                  << "\n"; // greet

        // to be continued...
        // end::before[]
        // tag::load[]
        // ...

        std::cout << "\nLoading " << LIBRARY_NAME << ".\n";

        using namespace boost::dll;
        shared_library lib(
            program_location().parent_path() / LIBRARY_NAME,
            load_mode::append_decorations);
        std::cout << "\nLoaded " << LIBRARY_NAME << ".\n";
        boost::openmethod::initialize(trace::from_env());

        std::cout << "cow meets wolf -> "
                  << meet(*std::make_unique<Cow>(), *std::make_unique<Wolf>())
                  << "\n"; // run
        std::cout << "wolf meets cow -> "
                  << meet(*std::make_unique<Wolf>(), *std::make_unique<Cow>())
                  << "\n"; // hunt

        auto make_tiger = lib.get<Animal*()>("make_tiger");
        std::cout << "cow meets tiger -> "
                  << meet(
                         *std::make_unique<Cow>(),
                         *std::unique_ptr<Animal>(make_tiger()))
                  << "\n"; // hunt
        // end::load[]

        // tag::unload[]
        // ...

        std::cout << "\nAfter unloading the shared library.\n";

        lib.unload();
        boost::openmethod::initialize(trace::from_env());

        std::cout << "cow meets wolf -> "
                  << meet(*std::make_unique<Cow>(), *std::make_unique<Wolf>())
                  << "\n"; // greet
        std::cout << "wolf meets cow -> "
                  << meet(*std::make_unique<Wolf>(), *std::make_unique<Cow>())
                  << "\n"; // greet
                           // tag::before[]
                           // tag::load[]
                           // tag::unload[]

        // end::before[]
        // end::load[]
        // end::unload[]

    } catch (const std::exception& ex) {
        std::cerr << "Exception: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}