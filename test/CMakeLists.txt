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
# instead define their own registry and #define BOOST_OPENMETHOD_DEFAULT_REGISTRY
# *before* the first inclusion of core.hpp (directly or via boost/openmethod.hpp);
# force-including a PCH that already pulled in core.hpp would defeat that
# override, so those files are detected (by scanning for the macro) and left
# without a PCH.
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

    file(READ ${test_cpp} test_cpp_contents)
    string(FIND "${test_cpp_contents}" "BOOST_OPENMETHOD_DEFAULT_REGISTRY" test_cpp_overrides_registry)

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

function(openmethod_compile_fail_test testname fail_regex)
    set(test_target "boost_openmethod-${testname}")
    add_library(${test_target} STATIC EXCLUDE_FROM_ALL "${testname}.cpp")
    target_link_libraries(${test_target} PRIVATE Boost::openmethod)
    add_test(
        NAME "${test_target}"
        COMMAND "${CMAKE_COMMAND}" --build ${CMAKE_BINARY_DIR} --target "${test_target}" --config $<CONFIG>)
    set_property(TEST "${test_target}" PROPERTY PASS_REGULAR_EXPRESSION "${fail_regex}")
    # Each of these tests runs `cmake --build` on the shared build tree. When
    # ctest runs tests in parallel, concurrent build-tool invocations race on
    # shared files (e.g. with the Visual Studio generator, every MSBuild
    # invocation rewrites ZERO_CHECK.tlog/ZERO_CHECK.lastbuildstate, and the
    # losers abort with MSB3491 before compiling anything, so the expected
    # diagnostic never appears in the output). Serialize the compile-fail
    # tests among themselves; they still run in parallel with the ordinary
    # tests, which do not touch the build tree.
    set_property(TEST "${test_target}" PROPERTY RESOURCE_LOCK openmethod_compile_fail)
endfunction()

openmethod_compile_fail_test(
    compile_fail_non_polymorphic_virtual_parameter "parameter is not a polymorphic class")
openmethod_compile_fail_test(
    compile_fail_non_polymorphic_virtual_ptr ".*")
openmethod_compile_fail_test(
    compile_fail_virtual_parameter_to_value "virtual_traits not specialized for type")
openmethod_compile_fail_test(
    compile_fail_virtual_ptr_different_registries "registry mismatch")
openmethod_compile_fail_test(
    compile_fail_virtual_ptr_other
    "virtual_ptr<> is required in overrider in same position as in method")
openmethod_compile_fail_test(
    compile_fail_virtual_ptr_ref_to_value "different virtual_ptr<> reference categories")
openmethod_compile_fail_test(
    compile_fail_virtual_ptr_shared_not_const "std::shared_ptr cannot be passed by non-const lvalue reference")
openmethod_compile_fail_test(
    compile_fail_virtual_ptr_value_to_ref "different virtual_ptr<> reference categories")
openmethod_compile_fail_test(
    compile_fail_virtual_parameter_private_base_macros "error")
openmethod_compile_fail_test(
    compile_fail_virtual_parameter_private_base_core "must be an unambiguous accessible base")
openmethod_compile_fail_test(
    compile_fail_repeated_inheritance "repeated inheritance")
openmethod_compile_fail_test(
    compile_fail_override_method_not_found "cannot find 'speak' method that accepts the same arguments as the overrider")

if (TARGET Boost::dll)
    add_subdirectory(dynamic_loading)
endif()
