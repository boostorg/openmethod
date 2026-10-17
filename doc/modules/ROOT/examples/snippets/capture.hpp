// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Harness for the snippets in this directory, never part of a tagged region:
// the reference pages show what a program would write, and the capture lets the
// test check that it wrote it.
//
// Note that the library's own `output` policy writes to the C `stderr` stream,
// which a streambuf redirect cannot intercept; only what an example prints
// itself is captured.

#ifndef BOOST_OPENMETHOD_SNIPPETS_CAPTURE_HPP
#define BOOST_OPENMETHOD_SNIPPETS_CAPTURE_HPP

#include <iostream>
#include <sstream>
#include <string>

// Redirects a standard stream for the duration of a scope.
template<std::ostream* Stream>
struct capture_stream {
    std::ostringstream captured;
    std::streambuf* previous = Stream->rdbuf(captured.rdbuf());

    ~capture_stream() {
        Stream->rdbuf(previous);
    }

    auto str() const -> std::string {
        return captured.str();
    }
};

using capture_cout = capture_stream<&std::cout>;
using capture_cerr = capture_stream<&std::cerr>;

#endif
