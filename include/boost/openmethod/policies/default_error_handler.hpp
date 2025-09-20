// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICY_VECTORED_ERROR_HPP
#define BOOST_OPENMETHOD_POLICY_VECTORED_ERROR_HPP

#include <boost/openmethod/registry.hpp>

#include <functional>
#include <variant>

namespace boost::openmethod::policies {

//! Calls a std::function with the error.
//!
//! Wraps the error in a @ref std::variant, and calls a `std::function` with it.
//! The function object is initialized to a function (@ref default_handler) that
//! writes a description of the error, using the @ref output policy, if it is
//! available in the registry.
//!
//! This is the error handler used by the default registry. In debug variants,
//! it writes an error message to `stderr`, then returns. In release variants,
//! no message is emitted. Any call by the library to the error policy is
//! immediately followed by a call to @ref abort.
//!
//! By default, the library is exception-agnostic: it is exception-safe, but it
//! does not throw exceptions by itself. The program may replace the default
//! handler with a function that throws an exception, possibly preventing
//! program termination. The @ref throw_error_handler policy can also be used to
//! enable exception throwing on a registry basis.

struct default_error_handler : error_handler {
    //! A @ref std::variant containing an instance of a subclass of @ref
    //! openmethod_error.
    using error_variant = std::variant<
        not_initialized_error, not_implemented_error, ambiguous_error,
        unknown_class_error, fast_perfect_hash_error, final_error,
        static_slot_error, static_stride_error>;

    //! The type of the error handler function object.
    using function_type = std::function<void(const error_variant& error)>;

    //! A model of @ref error_handler::fn.
    //!
    //! @tparam Registry The registry containing this policy.
    template<class Registry>
    class fn {
      public:
        //! Calls a function with the error object, wrapped in an @ref
        //! error_variant.
        //!
        //! @tparam Error A subclass of @ref openmethod_error.
        //! @param error The error object.
        template<class Error>
        static auto error(const Error& error) -> void {
            handler(error_variant(error));
        }

        //! Sets the function to be called to handle errors.
        //!
        //! Sets the error handler function to a new value, and returns the
        //! previous function.
        //!
        //! @param new_handler the new function.
        //! @return The previous function.
        static auto set(function_type new_handler) -> function_type {
            auto prev = handler;
            handler = new_handler;

            return prev;
        }

        //! The default error handler function.
        //!
        //! @param error A variant containing the error.
        //!
        //! If `Registry` contains an @ref output policy, writes a description
        //! of the error; otherwise, does nothing.
        static auto default_handler(const error_variant& error) -> void {
            if constexpr (Registry::has_output) {
                std::visit(
                    [](auto&& error) {
                        error.template write<Registry>(Registry::output::os);
                    },
                    error);
                Registry::output::os << "\n";
            }
        }

      private:
        static function_type
            handler; // Cannot be inline static because it confuses MSVC
    };
};

template<class Registry>
typename default_error_handler::function_type
    default_error_handler::fn<Registry>::handler = default_handler;

} // namespace boost::openmethod::policies

#endif
