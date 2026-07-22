// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_CLASSES_HPP
#define BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_CLASSES_HPP

#include <boost/config.hpp>
#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/std_unique_ptr.hpp>
#include <memory>

// BOOST_SYMBOL_VISIBLE gives these classes' RTTI default ELF visibility even
// in a hidden-visibility build (as used by the _hidden_vis CMake variant, and
// by the super-project's BoostRoot.cmake): their type_id can then unify
// across modules while each module's Registry::static_vptr<Class> - an
// ordinary template static, not covered by the registry-state export/import
// mechanism - stays a per-module copy. See test_shared_state in main.cpp,
// which exercises this by comparing vptrs and dispatching across modules.
struct BOOST_SYMBOL_VISIBLE Animal {
    virtual ~Animal() = default;
};

struct BOOST_SYMBOL_VISIBLE Dog : Animal {};

// Used by shared_overrider.hpp to reproduce the cross-module overrider-dedup
// scenario: an overrider for Cat is registered identically by two modules
// that share the registry state.
struct BOOST_SYMBOL_VISIBLE Cat : Animal {};

static auto make_dog() {
    return boost::openmethod::make_unique_virtual<Dog>();
}

static auto make_cat() {
    return boost::openmethod::make_unique_virtual<Cat>();
}

#endif