// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: a listed class must be complete

#include <boost/openmethod.hpp>

#if !BOOST_OPENMETHOD_HAS_REFLECTION

// Without reflection there is nothing to check; produce the expected
// diagnostic so the test passes under any configuration.
#error a listed class must be complete

#else

namespace app {

// Declared, never defined: a forward declaration left in the list by mistake.
struct Animal;

BOOST_OPENMETHOD_REGISTER(boost::openmethod::register_classes<{^^app::Animal}>);

} // namespace app

#endif

int main() {
    return 0;
}
