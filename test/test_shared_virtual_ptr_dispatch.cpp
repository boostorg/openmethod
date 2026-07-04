// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/interop/std_shared_ptr.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE shared_virtual_ptr_dispatch
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

    BOOST_OPENMETHOD_REGISTER(
        use_classes<Player, Warrior, Object, Axe, Bear, Registry>);

    using poke = method<
        BOOST_OPENMETHOD_ID(poke),
        auto(shared_virtual_ptr<Player, Registry>)->std::string, Registry>;

    BOOST_OPENMETHOD_REGISTER(typename poke::template override<
                              poke_bear<shared_virtual_ptr<Player, Registry>>>);

    using fight = method<
        BOOST_OPENMETHOD_ID(fight),
        auto(
            shared_virtual_ptr<Player, Registry>,
            shared_virtual_ptr<Object, Registry>,
            shared_virtual_ptr<Player, Registry>)
            ->std::string,
        Registry>;

    BOOST_OPENMETHOD_REGISTER(typename fight::template override<fight_bear<
                                  shared_virtual_ptr<Player, Registry>,
                                  shared_virtual_ptr<Object, Registry>,
                                  shared_virtual_ptr<Player, Registry>>>);

    initialize<Registry>();

    auto bear = make_shared_virtual<Bear, Registry>();
    auto warrior = make_shared_virtual<Warrior, Registry>();
    auto axe = make_shared_virtual<Axe, Registry>();

    BOOST_TEST(poke::fn(bear) == "growl");

    BOOST_TEST(fight::fn(warrior, axe, bear) == "kill bear");
}
