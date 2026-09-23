// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_ISL_MEMBER_OVERRIDER_LIB_HPP
#define BOOST_OPENMETHOD_TEST_ISL_MEMBER_OVERRIDER_LIB_HPP

#include <boost/config.hpp>
#include <boost/openmethod.hpp>

#if defined(OWNS_REGISTRY_STATE)
BOOST_OPENMETHOD_EXPORT_REGISTRY(boost::openmethod::default_registry);
#else
BOOST_OPENMETHOD_IMPORT_REGISTRY(boost::openmethod::default_registry);
#endif

struct BOOST_SYMBOL_VISIBLE Animal {
    virtual ~Animal() = default;
};

struct BOOST_SYMBOL_VISIBLE Dog : Animal {};

// A member method and a member overrider, both in a header, so both modules
// declare them and both register. Everything the _MEM macros emit is a class
// member, so the two modules must agree on it exactly; and the overrider,
// being one overrider rather than two, must end up registered once.
struct BOOST_SYMBOL_VISIBLE Zoo {
    BOOST_OPENMETHOD_MEM(
        poke, (boost::openmethod::virtual_ptr<Animal>), const char*);
};

class BOOST_SYMBOL_VISIBLE Keeper {
    BOOST_OPENMETHOD_OVERRIDE_MEM(
        Zoo::poke, (boost::openmethod::virtual_ptr<Dog>), const char*) {
        return "woof";
    }
};

#if defined(LIB_SOURCE)
#define LIB_API BOOST_SYMBOL_EXPORT
#else
#define LIB_API BOOST_SYMBOL_IMPORT
#endif

LIB_API auto lib_registry_state_id() -> const void*;

// Dispatch through the member method, performed inside the library.
LIB_API auto lib_poke(boost::openmethod::virtual_ptr<Animal> animal)
    -> const char*;

#endif
