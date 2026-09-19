// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// The two compilers disagree on the wording for this one: gcc says "cannot
// be overloaded with", clang says "class member cannot be redeclared" - no
// common substring, hence the `.*`.
// expected-error: .*

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
