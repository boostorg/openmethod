// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE virtual_ptr_dispatch
#include <boost/test/unit_test.hpp>

#include "test_util.hpp"

using boost::mp11::mp_list;
using namespace boost::openmethod;
using namespace boost::openmethod::detail;
using namespace game;

struct BOOST_OPENMETHOD_ID(poke);
struct BOOST_OPENMETHOD_ID(fight);

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_virtual_ptr_dispatch, Registry, policy_types<__COUNTER__>) {

    // Function-local static: registers on first pass through this
    // declaration, not before main. BOOST_OPENMETHOD_REGISTER is now
    // `inline`, which is illegal at block scope, so it's spelled out here.
    static use_classes<Player, Warrior, Object, Axe, Bear, Registry>
        BOOST_OPENMETHOD_GENSYM;

    using poke = method<
        BOOST_OPENMETHOD_ID(poke),
        auto(virtual_ptr<Player, Registry>)->std::string, Registry>;
    static typename poke::template override<
        poke_bear<virtual_ptr<Player, Registry>>>
        BOOST_OPENMETHOD_GENSYM;

    using fight = method<
        BOOST_OPENMETHOD_ID(fight),
        auto(
            virtual_ptr<Player, Registry>, virtual_ptr<Object, Registry>,
            virtual_ptr<Player, Registry>)
            ->std::string,
        Registry>;
    static typename fight::template override<fight_bear<
        virtual_ptr<Player, Registry>, virtual_ptr<Object, Registry>,
        virtual_ptr<Player, Registry>>>
        BOOST_OPENMETHOD_GENSYM;

    initialize<Registry>();

    Bear bear;
    BOOST_TEST(poke::fn(virtual_ptr<Player, Registry>(bear)) == "growl");

    Warrior warrior;
    Axe axe;
    BOOST_TEST(
        fight::fn(
            virtual_ptr<Player, Registry>(warrior),
            virtual_ptr<Object, Registry>(axe),
            virtual_ptr<Player, Registry>(bear)) == "kill bear");
}
