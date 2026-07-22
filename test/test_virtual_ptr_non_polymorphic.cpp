// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include <ostream>

#define BOOST_TEST_MODULE virtual_ptr_non_polymorphic
#include <boost/test/unit_test.hpp>
#include <boost/test/tools/output_test_stream.hpp>

using namespace boost::openmethod;

struct Animal {};

struct Dog : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

BOOST_OPENMETHOD(poke, (virtual_ptr<Animal>, std::ostream&), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (virtual_ptr<Dog>, std::ostream& os), void) {
    os << "bark";
}

BOOST_AUTO_TEST_CASE(test_virtual_ptr_non_polymorphic) {
    boost::openmethod::initialize();

    {
        boost::test_tools::output_test_stream os;
        Dog dog;
        auto vptr = virtual_ptr<Dog>::final(dog);
        poke(vptr, os);
        BOOST_CHECK(os.is_equal("bark"));
    }
}
