// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma GCC diagnostic ignored "-Wunused-function"

#define EXPORT_REGISTRY
#define INCLUDED_FROM "lib_registry.cpp"

#include "registry.hpp"
#include "classes.hpp"

#include <boost/openmethod/initialize.hpp>

#include <boost/dll/alias.hpp>

using namespace boost::openmethod;
namespace mp11 = boost::mp11;

#ifdef _WIN32
static_assert(std::is_same_v<default_registry::declspec, dllexport>);
#endif

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

BOOST_DLL_ALIAS(get_ids, registry_get_ids);

void registry_initialize() {
    boost::openmethod::initialize(trace::from_env());
}

BOOST_DLL_ALIAS(make_dog, registry_make_dog);
