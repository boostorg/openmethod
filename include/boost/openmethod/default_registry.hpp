// Copyright (c) 2017-2026 Jean-Louis Leroy
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
//! @ref BOOST_OPENMETHOD_DEFAULT_REGISTRY.
//! It contains the following policies:
//! @li @ref policies::std_rtti. Use standard RTTI.
//! @li @ref policies::fast_perfect_hash. Use a fast perfect hash function to
//!   map type ids to indices.
//! @li @ref policies::vptr_vector. Store v-table pointers in a @c std::vector.
//! @li @ref policies::default_error_handler. Write short diagnostic messages.
//! @li @ref policies::stderr_output. Write messages to @c stderr.
//!
//! If @ref BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
//! is defined, `default_registry` also includes the @ref runtime_checks policy.
//!
//! @note Use `BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS` with caution, as
//! inconsistent use of the macro can cause ODR violations. If defined, it must
//! be in all the translation units in the program that use `default_registry`,
//! including those pulled from libraries.
//!
//! For a program and its shared libraries to contribute to the same
//! `default_registry`, its state must be shared across the modules, with
//! @ref BOOST_OPENMETHOD_IMPORT_REGISTRY, @ref BOOST_OPENMETHOD_EXPORT_REGISTRY
//! and @ref BOOST_OPENMETHOD_INSTANTIATE_REGISTRY, as follows:
//! @code
//! // header, every translation unit of a client module:
//! BOOST_OPENMETHOD_IMPORT_REGISTRY(boost::openmethod::default_registry);
//! // header, every translation unit of the owning module:
//! BOOST_OPENMETHOD_EXPORT_REGISTRY(boost::openmethod::default_registry);
//! // exactly one .cpp of the owning module:
//! BOOST_OPENMETHOD_INSTANTIATE_REGISTRY(boost::openmethod::default_registry);
//! @endcode
struct default_registry :
    registry<
        policies::std_rtti, policies::fast_perfect_hash, policies::vptr_vector,
        policies::default_error_handler, policies::stderr_output
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
//! `indirect_registry` has its own state, separate from `default_registry`'s.
//! Share it across shared libraries exactly as for @ref default_registry,
//! naming `indirect_registry` in the macros.
//!
//! @see @ref policies::indirect_vptr
struct indirect_registry : default_registry::with<policies::indirect_vptr> {};

} // namespace boost::openmethod

// The library only tests BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS, it never
// defines it - that is up to the program. MrDocs extracts macros from
// `#define` directives, so give it one to extract. It is placed after
// `default_registry`, whose definition tests the macro, so that documenting it
// cannot change what is documented.
#ifdef __MRDOCS__
#ifndef BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
//! Enable runtime checks in @ref boost::openmethod::default_registry.
//!
//! May be defined by a program before including
//! `<boost/openmethod/default_registry.hpp>` (or any header that includes it,
//! like `<boost/openmethod.hpp>`) to enable runtime checks. See
//! @ref boost::openmethod::default_registry for details.
//!
//! @par Example
//!
//! Define the symbol before including the library, or on the compiler command
//! line:
//!
//! @code
//! #define BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
//! #include <boost/openmethod.hpp>
//! @endcode
//!
//! @note The error goes to the registry's
//! @ref boost::openmethod::policies::error_handler policy, which writes the
//! description shown in the comments; the program is then terminated. A
//! handler may throw instead, to keep the program running.
//!
//! The checks catch what @ref boost::openmethod::initialize cannot. Below,
//! `Bulldog` is never registered; nothing is amiss until a call passes one,
//! and only then is @ref boost::openmethod::missing_class reported:
//!
//! include:errors_missing_class_call.cpp#classes;use
//!
//! Without the checks the same call proceeds on a v-table pointer that was
//! never set up, and the behavior is undefined.
//!
//! @see [Registries and Policies](xref:ROOT:registries_and_policies.adoc)
#define BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS
#endif
#endif

#endif
