// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// This program is compiled with
// -DBOOST_OPENMETHOD_DEFAULT_REGISTRY=indirect_registry, so the registry
// under use is indirect_registry, and the INDIRECT macro pair applies.

// tag::content[]
// indirect_main.cpp

#define BOOST_OPENMETHOD_EXPORT_INDIRECT_REGISTRY

#include "animals.hpp"

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>
#include <boost/dll/shared_library.hpp>
#include <iostream>

using namespace boost::openmethod::aliases;

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Animal>, virtual_ptr<Animal>), std::string) {
    return "greet";
}

auto main() -> int {
    std::cout << "Before loading the shared library.\n";
    boost::openmethod::initialize();

    auto gracie = make_unique_virtual<Cow>();
    auto willy = make_unique_virtual<Wolf>();

    std::cout << "cow meets wolf -> " << meet(*gracie, *willy) << "\n"; // greet
    std::cout << "wolf meets cow -> " << meet(*willy, *gracie) << "\n"; // greet

    std::cout << "\nAfter loading the shared library.\n";

    boost::dll::shared_library lib(
        "boost_openmethod-indirect_shared",
        boost::dll::load_mode::rtld_global |
            boost::dll::load_mode::append_decorations);

    boost::openmethod::initialize();

    // The virtual_ptrs made before the reload still dispatch correctly:
    std::cout << "cow meets wolf -> " << meet(*gracie, *willy) << "\n"; // run
    std::cout << "wolf meets cow -> " << meet(*willy, *gracie) << "\n"; // hunt

    return 0;
}
// end::content[]
