// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: cannot find

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

class Animal {
  public:
    virtual ~Animal() = default;
};

class Dog : public Animal {};

// BOOST_OPENMETHOD_MEM emits everything - the tag, the guides, the call
// forwarder - with the access of the section it is used in. A private
// member method's guide is therefore unreachable from outside the class:
// LOCATE_METHOD's ADL-based lookup is a substitution failure, not an access
// error, so it reports the same "cannot find" diagnostic as a genuinely
// missing method.
class Zoo {
    BOOST_OPENMETHOD_MEM(poke, (virtual_ptr<Animal>), void);
};

class Handler {
    BOOST_OPENMETHOD_OVERRIDE_MEM(Zoo::poke, (virtual_ptr<Dog>), void) {
    }
};

int main() {
    return 0;
}
