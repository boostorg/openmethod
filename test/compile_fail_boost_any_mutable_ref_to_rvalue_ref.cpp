// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// Expected diagnostic, as a CMake regex (see CMakeLists.txt).
// expected-error: no matching

#include <string>
#include <utility>
#include <boost/any.hpp>

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/boost_any.hpp>

using namespace boost::openmethod;

struct Dog {
    std::string name;
};

BOOST_OPENMETHOD(bump, (virtual_<boost::any&>), std::string);

using bump_method =
    BOOST_OPENMETHOD_TYPE(bump, (virtual_<boost::any&>), std::string);

// Unlike std::any_cast, boost::any_cast binds an rvalue reference to the value
// stored in an lvalue `any`, which would let this overrider move the value out
// of an `any` the caller still owns. Moving the value out must go through a
// virtual_<boost::any&&> parameter.
//
// The overrider is registered via method<...>::override<Fn> because
// BOOST_OPENMETHOD_OVERRIDE cannot locate a method whose virtual parameter is
// a mutable lvalue reference to `any` - see test_dispatch_boost_any.cpp.
auto bump_dog(Dog&& dog) -> std::string {
    return std::move(dog.name);
}

BOOST_OPENMETHOD_REGISTER(bump_method::override<bump_dog>);

int main() {
    return 0;
}
