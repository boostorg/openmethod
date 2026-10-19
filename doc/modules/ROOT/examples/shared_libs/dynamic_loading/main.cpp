// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// tag::before[]
// main.cpp

#define OWNS_REGISTRY_STATE

#include "animals.hpp"

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/dll/shared_library.hpp>
#include <iostream>
#include <memory>
#include <string>

using namespace boost::openmethod;

BOOST_OPENMETHOD_INSTANTIATE_REGISTRY(boost::openmethod::default_registry);

BOOST_OPENMETHOD_CLASSES(Herbivore, Cow, Carnivore, Wolf);

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Animal>, virtual_ptr<Animal>), std::string) {
    return "greet";
}

// tag::load[]
// tag::unload[]
int main() {
    // end::load[]
    // end::unload[]

    try {
        std::cout << "Before loading the shared library.\n";

        boost::openmethod::initialize(trace::from_env());

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

        std::cout << "\nLoading shared object / DLL.\n";

        boost::dll::shared_library lib(
            "boost_openmethod-shared",
            boost::dll::load_mode::rtld_global |
                boost::dll::load_mode::append_decorations);

        boost::openmethod::initialize(trace::from_env());

        std::cout << "cow meets wolf -> "
                  << meet(*std::make_unique<Cow>(), *std::make_unique<Wolf>())
                  << "\n"; // do not greet, run
        std::cout << "wolf meets cow -> "
                  << meet(*std::make_unique<Wolf>(), *std::make_unique<Cow>())
                  << "\n"; // hunt

        auto make_tiger = lib.get<Animal*()>("make_tiger");
        std::cout << "cow meets tiger -> "
                  << meet(
                         *std::make_unique<Cow>(),
                         *std::unique_ptr<Animal>(make_tiger()))
                  << "\n"; // do not greet, run
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
        // end::unload[]
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
