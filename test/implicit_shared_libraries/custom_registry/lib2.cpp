// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// A second translation unit of the library that owns the registry state. It
// makes this a multi-TU owning module, the configuration that requires the
// exported declaration: without the one registry.hpp emits here, this
// translation unit would instantiate the state
// implicitly, and under -fvisibility=hidden that module-local copy would demote
// the merged symbol to local, leaving main.cpp unable to link.
//
// Like every translation unit of the owning module it defines
// OWNS_REGISTRY_STATE, so it declares the export rather than importing. It
// emits no definition: that belongs in exactly one .cpp, and lib.cpp is it.
#define OWNS_REGISTRY_STATE
#define LIB_SOURCE

#include "lib.hpp"
#include "../../test_classes.hpp"

using namespace boost::openmethod;

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Cow);

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Cow>), const char*) {
    return "moo";
}

auto lib_make_cow() -> unique_virtual_ptr<Animal> {
    return make_unique_virtual<Cow>();
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
