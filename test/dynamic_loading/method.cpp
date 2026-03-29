// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#define EXPORT_METHOD

#include "registry.hpp"
#include "method.hpp"

#include <boost/openmethod.hpp>
#include <boost/dll/alias.hpp>

#include <iostream>

using namespace boost::openmethod;
namespace mp11 = boost::mp11;

#ifdef _WIN32
static_assert(std::is_same_v<default_registry::declspec, dllimport>);
#endif

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Animal>), const char*) {
    return "?";
}

BOOST_DLL_ALIAS(get_ids, method_get_ids)
BOOST_DLL_ALIAS(get_fn, method_get_fn)
BOOST_DLL_ALIAS(make_dog, method_make_dog);
BOOST_DLL_ALIAS(call_speak, method_call_speak);
