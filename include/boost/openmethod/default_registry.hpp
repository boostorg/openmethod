// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_DEFAULT_REGISTRY_HPP
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY_HPP

#include <boost/openmethod/preamble.hpp>
#include <boost/openmethod/policies/std_rtti.hpp>
#include <boost/openmethod/policies/vptr_vector.hpp>
#include <boost/openmethod/policies/stderr_output.hpp>
#include <boost/openmethod/policies/fast_perfect_hash.hpp>
#include <boost/openmethod/policies/default_error_handler.hpp>

namespace boost::openmethod {

//! Default registry.
//!
//! `default_registry` is a predefined @ref registry, and the default value of
//! {{BOOST_OPENMETHOD_DEFAULT_REGISTRY}}.
//! It contains the following policies:
//! @li @ref policies::std_rtti: Use standard RTTI.
//! @li @ref policies::fast_perfect_hash: Use a fast perfect hash function to
//!   map type ids to indices.
//! @li @ref policies::vptr_vector: Store v-table pointers in a `std::vector`.
//! @li @ref policies::default_error_handler: Write short diagnostic messages.
//! @li @ref policies::stderr_output: Write messages to `stderr`.
//!
//! If
//! {{BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS}}
//! is defined, `default_registry` also includes the @ref runtime_checks policy.
//!
//! @note Use `BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS` with caution, as
//! inconsistent use of the macro can cause ODR violations. If defined, it must
//! be in all the translation units in the program that use `default_registry`,
//! including those pulled from libraries.
#if defined(_WIN32) || defined(__CYGWIN__)
# if defined(BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY)
//! The `dllvar` policy of @ref default_registry. Its type name is stable across
//! translation units, but it derives from @ref policies::dllexport in the
//! library that owns the registry state and from @ref policies::dllimport in
//! clients, so `default_registry` keeps a single shared symbol while the
//! export/import attribute flips per translation unit.
struct default_registry_dllvar : policies::dllexport {};
# elif defined(BOOST_OPENMETHOD_IMPORT_DEFAULT_REGISTRY)
struct default_registry_dllvar : policies::dllimport {};
# endif
#endif

struct default_registry
    : registry<
          policies::std_rtti, policies::fast_perfect_hash,
          policies::vptr_vector, policies::default_error_handler,
          policies::stderr_output
#if (defined(_WIN32) || defined(__CYGWIN__)) &&                                \
    (defined(BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY) ||                      \
     defined(BOOST_OPENMETHOD_IMPORT_DEFAULT_REGISTRY))
          ,
          default_registry_dllvar
#endif
#ifdef BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
          ,
          policies::runtime_checks
#endif
          > {
};

namespace detail {

static odr_check<default_registry> default_registry_odr_check_instance;

}

//! Indirect registry.
//!
//! `indirect_registry` is a predefined @ref registry that uses the same
//! policies as @ref default_registry, plus the @ref indirect_vptr policy.
//!
//! @see indirect_vptr.
struct indirect_registry : default_registry::with<policies::indirect_vptr> {};

} // namespace boost::openmethod

#endif
