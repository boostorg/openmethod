#
# Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
# Copyright (c) 2021 DMitry Arkhipov (grisumbras@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/boostorg/openmethod
#

message(STATUS "Boost.OpenMethod: building tests")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS)
endif()

# Custom target used by the boost super-project
if (NOT TARGET tests)
    add_custom_target(tests)
    set_property(TARGET tests PROPERTY FOLDER Dependencies)
endif()

# Replicate error flags from Jamfile
if (BOOST_OPENMETHOD_WARNINGS_AS_ERRORS)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 7)
            set(BOOST_OPENMETHOD_TEST_FLAGS "-Wall -Werror -Wno-unused-but-set-variable -Wno-maybe-uninitialized")
        else()
            set(BOOST_OPENMETHOD_TEST_FLAGS "-Wall -Werror -Wno-unused-but-set-variable")
        endif()
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "AppleClang")
        set(BOOST_OPENMETHOD_TEST_FLAGS "-Wall -Werror")
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
        if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
            set(BOOST_OPENMETHOD_TEST_FLAGS "-Wall -Werror -Wno-unused-but-set-variable")
        else()
            set(BOOST_OPENMETHOD_TEST_FLAGS "-Wall -Werror")
        endif()
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
        set(BOOST_OPENMETHOD_TEST_FLAGS "/W4 /WX /we4265")
    endif()

    # Print test configuration if running in CI
    # This is useful for debugging CI failures related to warnings which might be false positives
    if (DEFINED ENV{CI})
        message(STATUS "Boost.OpenMethod Tests - Compiler ID: ${CMAKE_CXX_COMPILER_ID} / ${CMAKE_CXX_COMPILER_VERSION}")
        if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT)
            message(STATUS "Boost.OpenMethod Tests - Compiler Frontend: ${CMAKE_CXX_COMPILER_FRONTEND_VARIANT}")
        endif()
        message(STATUS "Boost.OpenMethod Tests - Platform: ${CMAKE_SYSTEM_NAME} / ${CMAKE_SYSTEM_VERSION}")
        message(STATUS "Boost.OpenMethod Tests - C++ standard: ${CMAKE_CXX_STANDARD}")
        message(STATUS "Boost.OpenMethod Tests - Test error flags: ${BOOST_OPENMETHOD_TEST_FLAGS}")
    endif()
endif()

function(boost_openmethod_add_test name)
    add_test(NAME ${name} COMMAND ${name})
    if (WIN32)
        set_property(
            TEST ${name}
            PROPERTY ENVIRONMENT_MODIFICATION
                "PATH=path_list_prepend:$<TARGET_FILE_DIR:Boost::unit_test_framework>")
    endif()
endfunction()

# Precompiled headers: most test_*.cpp files just #include <boost/openmethod.hpp>
# (and friends) before anything else, so a shared PCH avoids re-parsing that
# (large, mp11-heavy) header once per test executable. A handful of files
# instead #define BOOST_OPENMETHOD_DEFAULT_REGISTRY *before* the first inclusion
# of core.hpp (directly or via boost/openmethod.hpp), and define the registry
# itself after it; force-including a PCH that already pulled in core.hpp would
# precede the #define and defeat the override, so those files are left without a
# PCH.
#
# Detected by scanning for the macro, or for an include of a header that carries
# the override on the file's behalf: test_capture_errors.hpp and
# test_checked_registry.hpp each own a whole recipe, so the tests including them
# name neither the macro nor their registry. Add another such header and this
# list has to learn about it - miss one and it still compiles, the macros simply
# bind to default_registry, and the test's expectations fail at run time.
set(BOOST_OPENMETHOD_TEST_PCH_HEADERS
    <boost/openmethod.hpp>
    <boost/openmethod/initialize.hpp>
    <boost/openmethod/interop/std_shared_ptr.hpp>
    <boost/openmethod/interop/std_unique_ptr.hpp>
)

set(boost_openmethod_pch_owner "")

file(GLOB test_cpp_files "test_*.cpp")

foreach(test_cpp ${test_cpp_files})
    get_filename_component(test ${test_cpp} NAME_WE)
    set(test_target "boost_openmethod-${test}")
    add_executable(${test_target} EXCLUDE_FROM_ALL ${test_cpp})
    target_link_libraries(${test_target} PRIVATE Boost::openmethod Boost::unit_test_framework)
    boost_openmethod_add_test(${test_target})
    add_dependencies(tests ${test_target})

    # test_slot_allocator.cpp instantiates the compiler for two dozen
    # registries, more sections than a COFF object holds by default.
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "MSVC")
        target_compile_options(${test_target} PRIVATE /bigobj)
    elseif (MINGW OR CYGWIN)
        target_compile_options(${test_target} PRIVATE -Wa,-mbig-obj)
    endif()

    file(READ ${test_cpp} test_cpp_contents)
    set(test_cpp_overrides_registry -1)
    foreach(marker "BOOST_OPENMETHOD_DEFAULT_REGISTRY" "test_capture_errors.hpp"
                   "test_checked_registry.hpp")
        string(FIND "${test_cpp_contents}" "${marker}" marker_pos)
        if (NOT marker_pos EQUAL -1)
            set(test_cpp_overrides_registry ${marker_pos})
        endif()
    endforeach()

    if (test_cpp_overrides_registry EQUAL -1)
        if (NOT boost_openmethod_pch_owner)
            target_precompile_headers(${test_target} PRIVATE ${BOOST_OPENMETHOD_TEST_PCH_HEADERS})
            set(boost_openmethod_pch_owner ${test_target})
        else()
            target_precompile_headers(${test_target} REUSE_FROM ${boost_openmethod_pch_owner})
        endif()
    endif()
endforeach()

add_executable(boost_openmethod-test_mix_release_debug EXCLUDE_FROM_ALL mix_release_debug/main.cpp mix_release_debug/lib.cpp)
target_link_libraries(boost_openmethod-test_mix_release_debug PRIVATE Boost::openmethod Boost::unit_test_framework)
boost_openmethod_add_test(boost_openmethod-test_mix_release_debug)
add_dependencies(tests boost_openmethod-test_mix_release_debug)

# Every compile-fail test runs `cmake --build` on the shared build tree. When
# that tree is out of date - someone edited a CMakeLists.txt - each of them
# re-runs CMake, and under `ctest -j N` those regenerations run concurrently and
# corrupt each other. On Ninja the losers die with
#
#     ninja: error: failed recompaction: No such file or directory
#     FAILED: build.ninja
#
# before compiling anything, so the expected diagnostic never appears and the
# test fails - reproducibly, in roughly two runs out of three. (Unix Makefiles
# survived the same stress, but the window is not specific to Ninja, so close it
# for every generator.) Bring the tree up to date once, in a CTest fixture, so
# no test regenerates while another is running. The target is empty: building it
# does nothing except make the generator refresh its build files first. CTest
# pulls a required fixture's setup test in automatically, so a narrower
# `ctest -R ...` selection still gets it.
add_custom_target(boost_openmethod-compile_fail_fixture)
add_test(
    NAME boost_openmethod-compile_fail_fixture
    COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR}
            --target boost_openmethod-compile_fail_fixture --config $<CONFIG>)
set_property(
    TEST boost_openmethod-compile_fail_fixture
    PROPERTY FIXTURES_SETUP openmethod_build_tree)

function(openmethod_compile_fail_test testname fail_regex)
    set(test_target "boost_openmethod-${testname}")
    add_library(${test_target} STATIC EXCLUDE_FROM_ALL "${testname}.cpp")
    target_link_libraries(${test_target} PRIVATE Boost::openmethod)
    add_test(
        NAME "${test_target}"
        COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target "${test_target}" --config $<CONFIG>)
    set_property(TEST "${test_target}" PROPERTY PASS_REGULAR_EXPRESSION "${fail_regex}")
    set_property(TEST "${test_target}" PROPERTY FIXTURES_REQUIRED openmethod_build_tree)
    # The fixture above is enough for Ninja and Makefiles, but not for the
    # Visual Studio generator, where the regeneration step is not the problem:
    # every MSBuild invocation walks the whole project dependency graph and
    # rewrites the .tlog/.lastbuildstate of each project it passes through -
    # ZERO_CHECK's and the dependencies' alike - so concurrent invocations
    # collide whether or not anything needs regenerating, and the losers abort
    # (MSB3374/3375/3491/3501) before compiling anything. Measured with VS 18
    # 2026 at `ctest -R compile_fail -j32`: 7-13 of 24 fail on every run without
    # the lock, 3/3 runs clean with it. Serialize them among themselves there;
    # they still run in parallel with the ordinary tests, which do not touch the
    # build tree. Windows users who want these parallel should configure with
    # -G Ninja, which is clean and ~14x faster here.
    if (CMAKE_GENERATOR MATCHES "Visual Studio")
        set_property(TEST "${test_target}" PROPERTY RESOURCE_LOCK openmethod_compile_fail)
    endif()
endfunction()

# Each compile_fail_*.cpp carries the diagnostic it expects in an
# `// expected-error: <regex>` comment, next to the code that provokes it.
file(GLOB compile_fail_cpp_files "compile_fail_*.cpp")

foreach(compile_fail_cpp ${compile_fail_cpp_files})
    get_filename_component(testname ${compile_fail_cpp} NAME_WE)
    file(READ ${compile_fail_cpp} compile_fail_cpp_contents)
    if (NOT compile_fail_cpp_contents MATCHES "//[ \t]*expected-error:[ \t]*([^\r\n]+)")
        message(FATAL_ERROR
            "${testname}.cpp has no `// expected-error: <regex>` comment")
    endif()
    openmethod_compile_fail_test(${testname} "${CMAKE_MATCH_1}")
endforeach()

if (TARGET Boost::dll)
    add_subdirectory(dynamic_loading)
endif()

# Implicitly (build-time) linked shared libraries. Needs no Boost::dll, since
# nothing is loaded at run time.
add_subdirectory(implicit_shared_libraries)
