// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// tag::content[]
// main.cpp

#include "animals.hpp"
#include <boost/openmethod/initialize.hpp>
#include <iostream>
#include <memory>
#include <string>

using namespace boost::openmethod::aliases;

struct Tiger : Carnivore {};

BOOST_OPENMETHOD_CLASSES(Tiger, Carnivore);
BOOST_OPENMETHOD_CLASSES(Animal, Herbivore, Cow, Carnivore, Wolf);

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Herbivore> a, virtual_ptr<Carnivore> b), std::string) {
    auto base = next(a, b);
    return "do not " + base + ", run";
}

BOOST_OPENMETHOD_OVERRIDE(
    meet, (virtual_ptr<Carnivore>, virtual_ptr<Herbivore>), std::string) {
    return "hunt";
}

auto main() -> int {
    boost::openmethod::initialize();

    std::unique_ptr<Animal> gracie(new Cow());
    std::unique_ptr<Animal> willy(new Wolf());
    std::unique_ptr<Animal> hobbes(new Tiger());

    std::cout << "cow meets wolf -> " << meet(*gracie, *willy)
              << "\n"; // do not greet, run
    std::cout << "wolf meets cow -> " << meet(*willy, *gracie) << "\n"; // hunt
    std::cout << "cow meets tiger -> " << meet(*gracie, *hobbes)
              << "\n"; // do not greet, run

    return 0;
}
// end::content[]
