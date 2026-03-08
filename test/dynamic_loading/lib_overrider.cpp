// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#define INCLUDED_FROM "lib_overrider.cpp"

#include "method.hpp"
#include <boost/dll/alias.hpp>

using namespace boost::openmethod;
namespace mp11 = boost::mp11;

BOOST_OPENMETHOD_OVERRIDE(speak, (virtual_ptr<Dog>), const char*) {
    return "woof";
}

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

BOOST_DLL_ALIAS(get_ids, overrider_get_ids)
BOOST_DLL_ALIAS(get_fn, overrider_get_fn)
