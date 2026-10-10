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

class Cat : private Animal {
  public:
    Animal& as_animal() {
        return *this;
    }
};

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>), void);

// A _MEM overrider is located the same way a free-macro overrider is -
// through BOOST_OPENMETHOD_DETAIL_LOCATE_METHOD, i.e. ADL/guide lookup, never
// through the core API's override<Fn> directly. Since Cat privately inherits
// Animal, virtual_ptr<Cat> does not convert to virtual_ptr<Animal> from
// outside Cat, so even the registry-relaxed guide fails to match and the
// diagnosis stops at "cannot find" - the more specific "must be an
// unambiguous accessible base" (see
// compile_fail_virtual_parameter_private_base_core.cpp) is only reachable by
// registering through the core API directly, bypassing guide lookup
// entirely, which no _MEM macro does.
class Handler {
    BOOST_OPENMETHOD_OVERRIDE_MEM(poke, (virtual_ptr<Cat>), void) {
    }
};

int main() {
    return 0;
}
