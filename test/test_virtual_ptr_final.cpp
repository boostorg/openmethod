// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE virtual_ptr_final
#include <boost/test/unit_test.hpp>

#include "test_util.hpp"

using boost::mp11::mp_list;
using namespace boost::openmethod;
using namespace boost::openmethod::detail;
using namespace game;

struct BOOST_OPENMETHOD_ID(poke);

BOOST_AUTO_TEST_CASE_TEMPLATE(
    test_virtual_ptr, Registry, policy_types<__COUNTER__>) {

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

    initialize<Registry>();

    using vptr_player = virtual_ptr<Player, Registry>;
    static_assert(detail::is_virtual_ptr<vptr_player>);
    using vptr_bear = virtual_ptr<Bear, Registry>;

    Player player;
    auto virtual_player = vptr_player::final(player);
    BOOST_TEST(&*virtual_player == &player);
    BOOST_TEST(
        (virtual_player.vptr() == Registry::template static_vptr<Player>));

    Bear bear;
    BOOST_TEST((&*vptr_bear::final(bear)) == &bear);
    BOOST_TEST((
        vptr_bear::final(bear).vptr() == Registry::template static_vptr<Bear>));

    BOOST_TEST(
        (vptr_player(bear).vptr() == Registry::template static_vptr<Bear>));

    vptr_bear virtual_bear_ptr(bear);

    struct upcast {
        static void fn(vptr_player) {
        }
    };

    upcast::fn(virtual_bear_ptr);
}
