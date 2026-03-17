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

using policy_ids_fn = const void**();
using method_fn = const void*();

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

DEFINE_GET_IDS()
DEFINE_MAKE_DOG()

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
    namespace fs = boost::filesystem;
    namespace ut = boost::unit_test;

    // b2 passes input-files as extra argv in alphabetical-path order (not
    // necessarily the order listed in the Jamfile). CMake puts all outputs in
    // one directory, found via program_location(), with no extra argv.
    auto& mts = ut::framework::master_test_suite();
    fs::path method_path, overrider_path;
    dll::load_mode::type load_flags;

    // Search argv for paths containing the library names (b2 mode).
    for (int i = 1; i < mts.argc; ++i) {
        std::string arg = mts.argv[i];
        if (arg.find("dl_test_method") != std::string::npos) {
            method_path = arg;
        } else if (arg.find("dl_test_overrider") != std::string::npos) {
            overrider_path = arg;
        }
    }

    if (!method_path.empty()) {
        // b2 mode: full versioned paths passed as argv (e.g. libfoo.so.1.91.0)
        load_flags = dll::load_mode::default_mode;
    } else {
        // CMake mode: libraries are alongside the executable, use decorations
        auto lib_dir = dll::program_location().parent_path();
        method_path = lib_dir / "boost_openmethod-dl_test_method";
        overrider_path = lib_dir / "boost_openmethod-dl_test_overrider";
        load_flags = dll::load_mode::append_decorations;
    }

    // Load all three shared libraries via Boost.DLL.
    // Use rtld_global for registry and method so their symbols (fn, policy
    // statics) are visible globally and win over any locally-instantiated
    // copies when the overrider library is subsequently loaded.
    dll::shared_library method_lib(method_path, load_flags);
    auto& method_get_ids = method_lib.get_alias<policy_ids_fn>("method_get_ids");
    auto& method_speak =
        method_lib.get_alias<const char*(virtual_ptr<Animal>)>("method_call_speak");
    auto& method_make_dog =
        method_lib.get_alias<unique_virtual_ptr<Animal>()>("method_make_dog");

    BOOST_TEST(same_ids(get_ids(), method_get_ids()));

    initialize();

    auto main_dog = make_dog();
    auto method_dog = method_make_dog();
    BOOST_TEST(main_dog.vptr() == method_dog.vptr());
#ifdef _WIN32
    BOOST_TEST(&typeid(*main_dog.get()) != &typeid(*method_dog.get()));
#endif
    BOOST_TEST(method_speak(main_dog) == "?");
    BOOST_TEST(method_speak(method_dog) == "?");

    dll::shared_library overrider_lib(overrider_path, load_flags);
    auto& overrider_get_ids = overrider_lib.get_alias<policy_ids_fn>("overrider_get_ids");
    auto& overrider_speak =
        overrider_lib.get_alias<const char*(virtual_ptr<Animal>)>("overrider_call_speak");
    auto& overrider_make_dog =
        overrider_lib.get_alias<unique_virtual_ptr<Animal>()>("overrider_make_dog");

    BOOST_TEST(same_ids(get_ids(), overrider_get_ids()));

    initialize();
    auto overrider_dog = overrider_make_dog();
    main_dog = make_dog(); // because its vptr was invalidated by initialize()
    method_dog = method_make_dog(); // ditto
    BOOST_TEST(main_dog.vptr() == overrider_dog.vptr());
#ifdef _WIN32
    BOOST_TEST(&typeid(*main_dog.get()) != &typeid(*overrider_dog.get()));
#endif
    BOOST_TEST(overrider_speak(main_dog) == "woof");
    BOOST_TEST(overrider_speak(method_dog) == "woof");
    BOOST_TEST(overrider_speak(overrider_dog) == "woof");
}
