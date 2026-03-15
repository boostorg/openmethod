// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define LIBRARY_NAME "indirect_shared"

// tag::content[]
// indirect_main.cpp

#include "animals.hpp"

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <iostream>

using namespace boost::openmethod::aliases;

struct Cow : Herbivore {};
struct Wolf : Carnivore {};

BOOST_OPENMETHOD_CLASSES(Animal, Herbivore, Cow, Wolf, Carnivore);

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Animal>, virtual_ptr<Animal>), std::string) {
    return "greet";
}

auto main() -> int {
    using namespace boost::openmethod::aliases;

    std::cout << "Before loading the shared library.\n";
    boost::openmethod::initialize();

    auto gracie = make_unique_virtual<Cow>();
    auto willy = make_unique_virtual<Wolf>();

    std::cout << "cow meets wolf -> " << meet(*gracie, *willy) << "\n"; // greet
    std::cout << "wolf meets cow -> " << meet(*willy, *gracie) << "\n"; // greet
    std::cout << "cow.vptr() = " << gracie.vptr() << "\n"; // 0x5d3121d22be8
    std::cout << "wolf.vptr() = " << willy.vptr() << "\n"; // 0x5d3121d22bd8

    std::cout << "\nAfter loading the shared library.\n";

    using namespace boost::dll;
    shared_library lib(
        program_location().parent_path() / LIBRARY_NAME,
        load_mode::append_decorations);

    boost::openmethod::initialize();

    std::cout << "cow meets wolf -> " << meet(*gracie, *willy) << "\n"; // run
    std::cout << "wolf meets cow -> " << meet(*willy, *gracie) << "\n"; // hunt
    std::cout << "cow.vptr() = " << gracie.vptr() << "\n"; // 0x5d3121d21998
    std::cout << "wolf.vptr() = " << willy.vptr() << "\n"; // 0x5d3121d21988

    return 0;
}
