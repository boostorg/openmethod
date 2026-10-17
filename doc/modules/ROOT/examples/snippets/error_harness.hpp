// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Harness for the error snippets, never part of a tagged region: the reference
// pages show the mistake and the operation that reports it, and nothing else.
//
// Each of those snippets lives in a translation unit of its own, so that one
// deliberate mistake cannot affect another and the examples can use the default
// registry -- which is what keeps a registry argument out of every line.

#ifndef BOOST_OPENMETHOD_SNIPPETS_ERROR_HARNESS_HPP
#define BOOST_OPENMETHOD_SNIPPETS_ERROR_HARNESS_HPP

#include <iostream>
#include <variant>

#include "capture.hpp"

// Thrown only to unwind out of an example: the library calls `abort` as soon as
// the error handler returns, and a handler may prevent that only by throwing.
struct reported {};

// Reports the error the way the default handler does, but on std::cerr. The
// `output` policy writes to the C `stderr` stream, which a streambuf redirect
// cannot intercept, so `capture_cerr` would see nothing otherwise.
template<class Registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY>
auto report_on_cerr() -> void {
    Registry::error_handler::set([](const auto& error) {
        std::visit(
            [](auto&& e) { e.template write<Registry>(std::cerr); }, error);
        std::cerr << "\n";
        throw reported{};
    });
}

// Runs `f`, swallowing the unwind that `report_on_cerr`'s handler throws.
template<class F>
auto reporting(F&& f) -> void {
    try {
        f();
    } catch (const reported&) {
    }
}

#endif
