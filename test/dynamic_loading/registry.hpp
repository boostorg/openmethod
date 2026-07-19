// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_REGISTRY_HPP
#define BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_REGISTRY_HPP

#include <boost/openmethod/preamble.hpp>

// The indirect variants select indirect_registry by defining
// BOOST_OPENMETHOD_DEFAULT_REGISTRY on the command line; the default variants
// leave it undefined. Pick the export/import macro pair matching the registry
// under test. This is done on every platform, not just Windows: the modules are
// built with hidden visibility in the super-project, where an implicitly
// instantiated registry state would not be shared across the shared objects, so
// the explicit-instantiation export/import (see default_registry.hpp) is needed
// on ELF too.
#if defined(EXPORT_REGISTRY)
#if defined(BOOST_OPENMETHOD_DEFAULT_REGISTRY)
#define BOOST_OPENMETHOD_EXPORT_INDIRECT_REGISTRY
#else
#define BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY
#endif
#else
#if defined(BOOST_OPENMETHOD_DEFAULT_REGISTRY)
#define BOOST_OPENMETHOD_IMPORT_INDIRECT_REGISTRY
#else
#define BOOST_OPENMETHOD_IMPORT_DEFAULT_REGISTRY
#endif
#endif

#include <boost/openmethod/default_registry.hpp>

// The registry under test. The build selects the variant by defining
// BOOST_OPENMETHOD_DEFAULT_REGISTRY on the command line (e.g. to
// ::boost::openmethod::indirect_registry); otherwise fall back to
// default_registry, matching core.hpp's own default. This must be set before
// the .cpp files include core.hpp.
#ifndef BOOST_OPENMETHOD_DEFAULT_REGISTRY
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY ::boost::openmethod::default_registry
#endif

using test_registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY;

// The single address that identifies the registry's shared state. It must be
// identical across all modules that share the registry (see registry::id()).
// inline (not static): registry.cpp never calls it, and an unreferenced static
// function trips MSVC's C4505 under /WX.
inline auto registry_state_id() -> const void* {
    return test_registry::id();
}

#endif
