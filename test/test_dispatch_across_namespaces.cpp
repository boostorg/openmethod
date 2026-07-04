// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE dispatch_across_namespaces
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

namespace animals {

class Animal {
  public:
    virtual ~Animal() {
    }
};

BOOST_OPENMETHOD(poke, (virtual_<const Animal&>), std::string);

} // namespace animals

namespace more_animals {

class Dog : public animals::Animal {};

BOOST_OPENMETHOD_CLASSES(Dog, animals::Animal);

BOOST_OPENMETHOD_OVERRIDE(poke, (const Dog&), std::string) {
    return "bark";
}

} // namespace more_animals

BOOST_AUTO_TEST_CASE(across_namespaces) {
    initialize();

    const animals::Animal& animal = more_animals::Dog();
    BOOST_TEST("bark" == poke(animal));
}
