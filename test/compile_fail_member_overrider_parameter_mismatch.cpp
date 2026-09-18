// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: must be an unambiguous accessible base

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

// A member overrider goes through the same override_aux/override_impl and
// validate_overrider_parameter machinery as a free-function one
// (BOOST_OPENMETHOD_OVERRIDE_FN is sugar over BOOST_OPENMETHOD_REGISTER and
// BOOST_OPENMETHOD_TYPE, not a new validation path), so a mismatched
// parameter is caught the same way.
class Handler {
    static auto poke_cat(virtual_ptr<Cat>) -> void {
    }

    BOOST_OPENMETHOD_OVERRIDE_FN(
        poke, (virtual_ptr<Animal>), void, &Handler::poke_cat);
};

int main() {
}
