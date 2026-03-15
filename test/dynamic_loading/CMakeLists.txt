# Copyright (c) 2018-2025 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

# lib_registry: exports the default registry's static policy state
add_library(boost_openmethod-dl_test_registry SHARED registry.cpp)
target_link_libraries(
    boost_openmethod-dl_test_registry
    PUBLIC Boost::openmethod PRIVATE Boost::dll)

# lib_method: exports the speak method; imports registry state from lib_registry
add_library(boost_openmethod-dl_test_method SHARED method.cpp)
target_link_libraries(
    boost_openmethod-dl_test_method
    PUBLIC Boost::openmethod boost_openmethod-dl_test_registry PRIVATE Boost::dll)

# lib_overrider: adds a Dog overrider; imported at runtime via Boost.DLL
add_library(boost_openmethod-dl_test_overrider SHARED overrider.cpp)
target_link_libraries(
    boost_openmethod-dl_test_overrider
    PRIVATE Boost::openmethod boost_openmethod-dl_test_method Boost::dll)

# Test executable: links against lib_method (and lib_registry transitively);
# dynamically loads lib_overrider at runtime.
add_executable(
    boost_openmethod-test_dynamic_loading main.cpp)
target_link_libraries(
    boost_openmethod-test_dynamic_loading
    PUBLIC Boost::openmethod boost_openmethod-dl_test_registry
    PRIVATE Boost::openmethod Boost::unit_test_framework Boost::dll)
add_dependencies(
    boost_openmethod-test_dynamic_loading
    boost_openmethod-dl_test_registry
    boost_openmethod-dl_test_method
    boost_openmethod-dl_test_overrider)

# On MinGW/GCC+Windows, BOOST_DLL_AUTO_ALIAS only emits a declaration without
# a definition (relying on --export-all-symbols). Force full alias instantiation
# so the alias pointer variables are actually defined and exported.
if(MINGW)
    foreach(target
            boost_openmethod-dl_test_registry
            boost_openmethod-dl_test_method
            boost_openmethod-dl_test_overrider)
        target_compile_definitions(
            ${target} PRIVATE BOOST_DLL_FORCE_ALIAS_INSTANTIATION)
    endforeach()
endif()

# Put all outputs in the same directory so Boost.DLL can locate the shared
# libraries relative to the test executable at runtime.
# ENABLE_EXPORTS is required so that symbols from each shared library are
# visible to other shared libraries loaded at runtime (e.g. boost_openmethod-dl_test_overrider
# can resolve symbols from boost_openmethod-dl_test_method/boost_openmethod-dl_test_registry).
# CXX_VISIBILITY_PRESET must be "default" so that the template static variables
# used as the shared registry state (policy statics) are exported with DEFAULT
# ELF visibility. With hidden visibility (as set by BoostRoot.cmake in the
# super-project build) they become UNIQUE HIDDEN and are not deduplicated by
# the dynamic linker, breaking cross-DSO state sharing.
foreach(
    target
        boost_openmethod-dl_test_registry
        boost_openmethod-dl_test_method
        boost_openmethod-dl_test_overrider
        boost_openmethod-test_dynamic_loading)
    set_target_properties(
        ${target}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            ENABLE_EXPORTS ON
            CXX_VISIBILITY_PRESET default
            VISIBILITY_INLINES_HIDDEN OFF)
endforeach()

add_test(
    NAME boost_openmethod-test_dynamic_loading-build
    COMMAND
        ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}
        --target boost_openmethod-test_dynamic_loading
        --config $<CONFIG>)
set_tests_properties(
    boost_openmethod-test_dynamic_loading-build
    PROPERTIES FIXTURES_SETUP boost_openmethod-test_dynamic_loading-fixture)

add_test(
    NAME boost_openmethod-test_dynamic_loading
    COMMAND boost_openmethod-test_dynamic_loading
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
set_tests_properties(
    boost_openmethod-test_dynamic_loading
    PROPERTIES FIXTURES_REQUIRED boost_openmethod-test_dynamic_loading-fixture)
