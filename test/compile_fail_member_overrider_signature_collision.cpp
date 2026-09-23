// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// The three compilers word this differently - gcc "cannot be overloaded
// with", clang "class member cannot be redeclared", MSVC C2556 "overloaded
// function differs only by return type" - but all three name the accessor
// whose overload set the collision happens in, so match that. A bare `.*`
// would match anything, and since the test asserts only
// PASS_REGULAR_EXPRESSION - CTest ignores the exit status once that is set -
// it would pass even if the file compiled cleanly.
// expected-error: boost_openmethod_overrider_key

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

class Animal {
  public:
    virtual ~Animal() = default;
};

class Dog : public Animal {};

BOOST_OPENMETHOD(pay, (virtual_ptr<Dog>), double);
BOOST_OPENMETHOD(greet, (virtual_ptr<Dog>), double);

// A _MEM overrider's body and the accessor that finds its key are each
// named once, overloaded purely on the exact (return type, parameter list) -
// never on which method they override, since a qualified method name cannot
// be pasted into a new declaration (see macros.hpp). Two overriders of
// *different* methods, with an identical signature, therefore redeclare the
// same overload in one class.
class Handler {
    BOOST_OPENMETHOD_OVERRIDE_MEM(pay, (virtual_ptr<Dog>), double) {
        return 1.0;
    }
    BOOST_OPENMETHOD_OVERRIDE_MEM(greet, (virtual_ptr<Dog>), double) {
        return 2.0;
    }
};

int main() {
    return 0;
}
