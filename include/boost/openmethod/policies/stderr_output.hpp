// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_POLICIES_STANDARD_ERROR_OUTPUT_HPP
#define BOOST_OPENMETHOD_POLICIES_STANDARD_ERROR_OUTPUT_HPP

#include <boost/openmethod/preamble.hpp>
#include <boost/openmethod/detail/ostdstream.hpp>

namespace boost::openmethod {

namespace policies {

//! @ref Writes to the C standard error stream.
//!
//! `stderr_output` writes to standard error using the C API.
struct stderr_output : output {
    //! An OutputFn metafunction.
    template<class Registry>
    struct fn {
      public:
        struct state {
            detail::ostderr os;
        };

        [[deprecated]] inline static detail::ostderr os;

        static auto& stream() {
            return Registry::state().template policy<stderr_output>().os;
        }
    };
};

} // namespace policies
} // namespace boost::openmethod

#endif
