// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_IMPLICIT_SHARED_LIBRARIES_NAMED_STATIC_RTTI_REGISTRY_HPP
#define BOOST_OPENMETHOD_TEST_IMPLICIT_SHARED_LIBRARIES_NAMED_STATIC_RTTI_REGISTRY_HPP

#include <boost/config.hpp>

struct static_registry;
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY static_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/policies/named_static_rtti.hpp>

struct BOOST_SYMBOL_VISIBLE static_registry :
    boost::openmethod::registry<
        boost::openmethod::policies::named_static_rtti> {};

// The registry state is shared exactly as in the custom_registry variant.
#if defined(OWNS_REGISTRY_STATE)
BOOST_OPENMETHOD_EXPORT_REGISTRY(static_registry);
#else
BOOST_OPENMETHOD_IMPORT_REGISTRY(static_registry);
#endif

#endif
