# Copyright (c) 2018-2025 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

# lib_registry: exports the default registry's static policy state
add_library(dl_test_registry SHARED EXCLUDE_FROM_ALL lib_registry.cpp)
target_compile_definitions(
    dl_test_registry PRIVATE BOOST_OPENMETHOD_DYNAMIC_LOADING_REGISTRY_EXPORTS)
target_link_libraries(dl_test_registry PUBLIC Boost::openmethod PRIVATE Boost::dll)

# lib_method: exports the speak method; imports registry state from lib_registry
add_library(dl_test_method SHARED EXCLUDE_FROM_ALL lib_method.cpp)
target_compile_definitions(
    dl_test_method PRIVATE BOOST_OPENMETHOD_DYNAMIC_LOADING_METHOD_EXPORTS)
target_link_libraries(dl_test_method PUBLIC Boost::openmethod dl_test_registry PRIVATE Boost::dll)

# lib_overrider: adds a Dog overrider; imported at runtime via Boost.DLL
add_library(dl_test_overrider SHARED EXCLUDE_FROM_ALL lib_overrider.cpp)
target_link_libraries(dl_test_overrider PRIVATE Boost::openmethod dl_test_method Boost::dll)

# Test executable: links against lib_method (and lib_registry transitively);
# dynamically loads lib_overrider at runtime.
add_executable(
    boost_openmethod-test_dynamic_loading EXCLUDE_FROM_ALL main.cpp)
target_link_libraries(
    boost_openmethod-test_dynamic_loading
    PRIVATE Boost::openmethod Boost::unit_test_framework Boost::dll)

# Put all outputs in the same directory so Boost.DLL can locate the shared
# libraries relative to the test executable at runtime.
# ENABLE_EXPORTS is required so that symbols from each shared library are
# visible to other shared libraries loaded at runtime (e.g. dl_test_overrider
# can resolve symbols from dl_test_method/dl_test_registry).
# CXX_VISIBILITY_PRESET must be "default" so that the template static variables
# used as the shared registry state (policy statics) are exported with DEFAULT
# ELF visibility. With hidden visibility (as set by BoostRoot.cmake in the
# super-project build) they become UNIQUE HIDDEN and are not deduplicated by
# the dynamic linker, breaking cross-DSO state sharing.
foreach(
    target dl_test_registry dl_test_method dl_test_overrider
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
    NAME boost_openmethod-test_dynamic_loading
    COMMAND boost_openmethod-test_dynamic_loading)
add_dependencies(
    tests boost_openmethod-test_dynamic_loading dl_test_overrider)
