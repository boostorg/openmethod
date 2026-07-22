// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// tag::content[]
// extensions.cpp

// end::content[]

// static_extensions.cpp and indirect_extensions.cpp reuse this file, defining
// a different export/import macro before including it.
#if !defined(BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY) &&                      \
    !defined(BOOST_OPENMETHOD_IMPORT_INDIRECT_REGISTRY)
// tag::content[]
#define BOOST_OPENMETHOD_IMPORT_DEFAULT_REGISTRY
// end::content[]
#endif

// tag::content[]

#include "animals.hpp"

using namespace boost::openmethod;

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Herbivore> a, virtual_ptr<Carnivore> b), std::string) {
    auto p = BOOST_OPENMETHOD_TYPE(
        meet, (virtual_ptr<Animal>, virtual_ptr<Animal>),
        std::string)::next<fn>;
// end::content[]
    BOOST_ASSERT(p);
    BOOST_ASSERT(p(a, b) == "greet");
// tag::content[]
    return "run";
}

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Carnivore>, virtual_ptr<Herbivore>), std::string) {
    return "hunt";
}

struct Tiger : Carnivore {};

BOOST_OPENMETHOD_CLASSES(Tiger, Carnivore);

extern "C" {
BOOST_SYMBOL_EXPORT auto make_tiger() -> Animal* {
// end::content[]
    BOOST_ASSERT(default_registry::static_vptr<Carnivore> != nullptr);
// tag::content[]
    return new Tiger;
}
}
// end::content[]
