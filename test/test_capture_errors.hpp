// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_CAPTURE_ERRORS_HPP
#define BOOST_OPENMETHOD_TEST_CAPTURE_ERRORS_HPP

// This header owns the whole registry recipe for the tests that capture
// diagnostics: the declaration, the BOOST_OPENMETHOD_DEFAULT_REGISTRY
// definition, the library include, and `test_registry` itself. Including it
// first - before anything that pulls in core.hpp - is all a test has to do,
// and there is no ordering left for a caller to get wrong.
struct test_registry;
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY test_registry

#include <boost/openmethod.hpp>

#include <sstream>
#include <variant>

struct capture_output : boost::openmethod::policies::output {
    template<class Registry>
    struct fn {
        inline static std::ostringstream os;
        static auto& stream() {
            return os;
        }
    };
};

struct test_registry :
    boost::openmethod::default_registry::with<capture_output> {};

template<class Registry>
struct capture_errors {
    using error_handler = typename Registry::error_handler;
    using output = typename Registry::output;

    capture_errors() {
        prev = error_handler::set(
            [this](const typename error_handler::error_variant& error) {
                prev(error);
                std::visit([](auto&& arg) { throw arg; }, error);
            });
    }

    ~capture_errors() {
        error_handler::set(prev);
        output::os.str("");
    }

    auto operator()() const {
        return output::os.str();
    }

    typename error_handler::function_type prev;
};

#endif
