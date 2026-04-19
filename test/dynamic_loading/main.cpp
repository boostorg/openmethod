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
#include <boost/filesystem.hpp>

#include <iomanip>
#include <stdexcept>

using namespace boost::openmethod;
namespace mp11 = boost::mp11;

using policy_ids_fn = const void**();
using method_fn = const void*();

BOOST_OPENMETHOD_CLASSES(Animal, Dog);

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

    // Load all three shared libraries via Boost.DLL.
    // Use rtld_global for registry and method so their symbols (fn, policy
    // statics) are visible globally and win over any locally-instantiated
    // copies when the overrider library is subsequently loaded.

    constexpr auto load_mode = dll::load_mode::rtld_global;

    namespace bfs = boost::filesystem;

    // On Windows, load the method DLL via its already-imported symbol to get
    // exactly the same module (avoid a second copy from the plain-named DLL).
    // On POSIX/macOS, locate libraries by scanning the executable's directory.
#ifdef _WIN32
    auto method_path = dll::symbol_location_ptr(get_fn());
    auto search_dir = method_path.parent_path();
#else
    auto search_dir = dll::program_location().parent_path();
#endif

    auto find_lib = [&](const char* name_fragment) {
        for (auto& entry : bfs::directory_iterator(search_dir)) {
            auto fname = entry.path().filename().string();
            if (fname.find(name_fragment) == std::string::npos)
                continue;
#ifdef _WIN32
            if (entry.path().extension() != ".dll")
                continue;
#else
            auto ext = entry.path().extension().string();
            if (fname.find(".so") == std::string::npos && ext != ".dylib")
                continue;
#endif
            return entry.path();
        }
        throw std::runtime_error(std::string("lib not found: ") + name_fragment);
    };

#ifdef _WIN32
    dll::shared_library method_lib(method_path, load_mode);
#else
    dll::shared_library method_lib(find_lib("method"), load_mode);
#endif
    auto& method_get_ids =
        method_lib.get_alias<policy_ids_fn>("method_get_ids");
    auto& method_speak = method_lib.get_alias<const char*(virtual_ptr<Animal>)>(
        "method_call_speak");
    auto& method_make_dog =
        method_lib.get_alias<unique_virtual_ptr<Animal>()>("method_make_dog");
    auto& method_get_fn = method_lib.get_alias<method_fn>("method_get_fn");

    BOOST_TEST(same_ids(get_ids(), method_get_ids()));
    BOOST_TEST(get_fn() == method_get_fn());

    initialize();

    auto main_dog = make_dog();
    auto method_dog = method_make_dog();
    BOOST_TEST(main_dog.vptr() == method_dog.vptr());
#ifdef _WIN32
    {
        Animal* p1 = main_dog.get();
        Animal* p2 = method_dog.get();
        BOOST_TEST(&typeid(*p1) != &typeid(*p2));
    }
#endif
    BOOST_TEST(method_speak(main_dog) == "?");
    BOOST_TEST(method_speak(method_dog) == "?");

    dll::shared_library overrider_lib(find_lib("overrider"), load_mode);
    auto overrider_get_ids =
        overrider_lib.get_alias<policy_ids_fn>("overrider_get_ids");
    auto overrider_speak =
        overrider_lib.get_alias<const char*(virtual_ptr<Animal>)>(
            "overrider_call_speak");
    auto overrider_make_dog =
        overrider_lib.get_alias<unique_virtual_ptr<Animal>()>(
            "overrider_make_dog");
    auto overrider_get_fn =
        overrider_lib.get_alias<method_fn>("overrider_get_fn");

    BOOST_TEST(same_ids(get_ids(), overrider_get_ids()));
    BOOST_TEST(get_fn() == overrider_get_fn());

    initialize();
    auto overrider_dog = overrider_make_dog();
    main_dog = make_dog(); // because its vptr was invalidated by initialize()
    method_dog = method_make_dog(); // ditto
    BOOST_TEST(main_dog.vptr() == overrider_dog.vptr());
    BOOST_TEST(overrider_speak(main_dog) == "woof");
    BOOST_TEST(overrider_speak(method_dog) == "woof");
    BOOST_TEST(overrider_speak(overrider_dog) == "woof");
}
