// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define EXPORT_REGISTRY

#include "registry.hpp"
#include "classes.hpp"

#include <boost/openmethod/initialize.hpp>

using namespace boost::openmethod;
namespace mp11 = boost::mp11;

#if defined(_WIN32) || defined(__CYGWIN__)
#include <boost/config.hpp>
static_assert(std::is_same_v<default_registry::declspec, dllexport>);
#else
#include <boost/dll/alias.hpp>
#endif

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

extern "C" {
    BOOST_SYMBOL_EXPORT const void* registry_get_ids = (const void*)&get_ids;
    BOOST_SYMBOL_EXPORT const void* registry_make_dog = (const void*)&make_dog;
}

void registry_initialize() {
    boost::openmethod::initialize(trace::from_env());
}
