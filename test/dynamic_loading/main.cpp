// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_TEST_MODULE dynamic_loading
#include <boost/test/unit_test.hpp>

#define INCLUDED_FROM "main.cpp"

#include "registry.hpp"
#include "method.hpp"

#include <boost/openmethod/initialize.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/runtime_symbol_info.hpp>

#include <iomanip>

using namespace boost::openmethod;
namespace mp11 = boost::mp11;

constexpr auto n_policies = mp11::mp_size<default_registry::policy_list>::value;

using policy_ids_fn = const void**();
using method_fn = const void*();

bool same_ids(const void** ids1, const void** ids2) {
    using std::setw;
    BOOST_TEST_MESSAGE(
        setw(60) << "registry state" << ": " << *ids1++ << " " << *ids2++);

    int diffs = 0;

    mp11::mp_for_each<default_registry::policy_list>([&](auto p) {
        using P = decltype(p);

        if constexpr (detail::has_id<default_registry::policy<P>>) {
            BOOST_TEST_MESSAGE(
                setw(60) << boost::core::demangle(typeid(P).name()) << ": "
                         << *ids1 << " " << *ids2);

            if (*ids1 != *ids2) {
                ++diffs;
            }

            ++ids1;
            ++ids2;
        }
    });

    return diffs == 0;
}

BOOST_AUTO_TEST_CASE(test_shared_state) {
    namespace dll = boost::dll;
    auto lib_dir = dll::program_location().parent_path();

    // Load all three shared libraries via Boost.DLL.
    // Use rtld_global for registry and method so their symbols (fn, policy
    // statics) are visible globally and win over any locally-instantiated
    // copies when the overrider library is subsequently loaded.
#ifdef _WIN32
    constexpr auto global_mode = dll::load_mode::append_decorations;
#else
    constexpr auto global_mode =
        dll::load_mode::append_decorations /*| dll::load_mode::rtld_global*/;
#endif
    dll::shared_library method_lib(
        lib_dir / "boost_openmethod-dl_test_method", global_mode);
    auto& method_get_ids =
        method_lib.get_alias<policy_ids_fn>("method_get_ids");

    BOOST_TEST(same_ids(get_ids(), method_get_ids()));

    auto& speak = method_lib.get_alias<const char*()>("call_speak");
    initialize();
    BOOST_TEST(speak() == "?");

    dll::shared_library overrider_lib(
        lib_dir / "boost_openmethod-dl_test_overrider",
        dll::load_mode::append_decorations);
    auto& overrider_get_ids =
        overrider_lib.get_alias<policy_ids_fn>("overrider_get_ids");
    auto& method_get_fn = method_lib.get_alias<method_fn>("method_get_fn");
    auto& overrider_get_fn =
        overrider_lib.get_alias<method_fn>("overrider_get_fn");
    BOOST_TEST(same_ids(get_ids(), overrider_get_ids()));
    BOOST_TEST(method_get_fn() == overrider_get_fn());

    initialize();
    BOOST_TEST(speak() == "woof");
}
