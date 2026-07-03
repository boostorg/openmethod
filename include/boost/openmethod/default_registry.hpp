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
struct default_registry
    : registry<
          policies::std_rtti, policies::fast_perfect_hash,
          policies::vptr_vector, policies::default_error_handler,
          policies::stderr_output
#ifdef BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
          ,
          policies::runtime_checks
#endif
          > {
};

// On Windows, share the registry state across modules via an explicit
// instantiation of detail::static_st: the owning module dllexport-s the
// instantiation definition, clients declare a dllimport-ed instantiation
// ("extern template") so they reference the owner's symbol instead of
// instantiating their own copy. The state lives in static_st<registry_type>
// (the registry<...> base class), not static_st<default_registry>.
//
// An explicit instantiation definition may appear only once in the program,
// so BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY must be compiled in exactly one
// translation unit of the owning module.
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY)
template struct BOOST_SYMBOL_EXPORT
    detail::static_st<default_registry::registry_type>;
#elif defined(BOOST_OPENMETHOD_IMPORT_DEFAULT_REGISTRY)
extern template struct BOOST_SYMBOL_IMPORT
    detail::static_st<default_registry::registry_type>;
#endif
#endif

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

// indirect_registry has its own state (its policy list differs from
// default_registry's), shared across modules with its own macro pair,
// BOOST_OPENMETHOD_{EXPORT,IMPORT}_INDIRECT_REGISTRY. As with the default
// registry, the EXPORT macro must be compiled in exactly one translation unit
// of the owning module.
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(BOOST_OPENMETHOD_EXPORT_INDIRECT_REGISTRY)
template struct BOOST_SYMBOL_EXPORT
    detail::static_st<indirect_registry::registry_type>;
#elif defined(BOOST_OPENMETHOD_IMPORT_INDIRECT_REGISTRY)
extern template struct BOOST_SYMBOL_IMPORT
    detail::static_st<indirect_registry::registry_type>;
#endif
#endif

} // namespace boost::openmethod

#endif
