#
# Copyright (c) 2019 Vinnie Falco (vinnie.falco@gmail.com)
# Copyright (c) 2021 DMitry Arkhipov (grisumbras@gmail.com)
# Copyright (c) 2022 Alan de Freitas (alandefreitas@gmail.com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/boostorg/openmethod
#

#-------------------------------------------------
#
# Project
#
#-------------------------------------------------

cmake_minimum_required(VERSION 3.8...3.20)

set(BOOST_OPENMETHOD_VERSION 1)

if (BOOST_SUPERPROJECT_VERSION)
    set(BOOST_OPENMETHOD_VERSION ${BOOST_SUPERPROJECT_VERSION})
endif ()

project(boost_openmethod VERSION "${BOOST_OPENMETHOD_VERSION}" LANGUAGES CXX)

set(BOOST_OPENMETHOD_IS_ROOT OFF)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(BOOST_OPENMETHOD_IS_ROOT ON)
endif ()
set(__ignore__ ${CMAKE_C_COMPILER})

#-------------------------------------------------
#
# Options
#
#-------------------------------------------------

option(
    BOOST_OPENMETHOD_BUILD_TESTS
    "Build boost::openmethod tests even if BUILD_TESTING is OFF"
    ${BOOST_OPENMETHOD_IS_ROOT})

if (BUILD_TESTING)
    set(BOOST_OPENMETHOD_BUILD_TESTS ON)
endif ()

option(
    BOOST_OPENMETHOD_BUILD_EXAMPLES
    "Build boost::openmethod examples"
    ${BOOST_OPENMETHOD_IS_ROOT})
option(
    BOOST_OPENMETHOD_MRDOCS_BUILD
    "Build the target for MrDocs: see mrdocs.yml"
    OFF)
option(
    BOOST_OPENMETHOD_WARNINGS_AS_ERRORS
    "Treat warnings as errors"
    OFF)
option(
    BOOST_OPENMETHOD_ENABLE_REFLECTION
    "Build the tests and examples with C++26 reflection enabled"
    OFF)

# C++26 reflection (P2996). The library detects it on its own, from
# __cpp_impl_reflection; this only arranges for the tests and examples to be
# built in a mode where the compiler provides it, which needs both C++26 and, on
# GCC, an opt-in flag. The options are added to this directory - every test,
# example and snippet, the shared-library ones in the subdirectories included -
# rather than to CMAKE_CXX_FLAGS, because CMake probes the compiler before
# CMAKE_CXX_STANDARD takes effect and GCC rejects -freflection under any other
# standard.
set(BOOST_OPENMETHOD_REFLECTION_OPTIONS "")

if (BOOST_OPENMETHOD_ENABLE_REFLECTION)
    # The probe passes its own -std flag, and try_compile appends the one it
    # derives from CMAKE_CXX_STANDARD *after* the definitions (policy CMP0067),
    # where it would win: a configure that sets the standard - a preset, a
    # toolchain file, a consuming project - would fail both candidates and stop
    # at the FATAL_ERROR below. Take the variables out of the probe's way, and
    # put them back after. Set to nothing rather than unset: a normal variable
    # hides a cache one - a -D on the command line - where unset would expose
    # it, and try_compile leaves an empty standard alone. The probe source is
    # the one b2 uses (config/Jamfile); like b2, compile it only - it has no
    # main - which the target type arranges.
    foreach(var CMAKE_CXX_STANDARD CMAKE_CXX_EXTENSIONS CMAKE_TRY_COMPILE_TARGET_TYPE)
        set(BOOST_OPENMETHOD_SAVED_${var} "${${var}}")
        set(${var} "")
    endforeach()
    set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

    foreach(candidate "-std=c++26" "-std=c++26;-freflection")
        unset(BOOST_OPENMETHOD_HAS_REFLECTION CACHE)
        try_compile(
            BOOST_OPENMETHOD_HAS_REFLECTION
            "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp"
            SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/config/has_reflection.cpp"
            COMPILE_DEFINITIONS ${candidate})

        if (BOOST_OPENMETHOD_HAS_REFLECTION)
            set(BOOST_OPENMETHOD_REFLECTION_OPTIONS ${candidate})
            break()
        endif()
    endforeach()

    # An empty one is unset, not set to nothing, which a target would take for
    # an invalid standard.
    foreach(var CMAKE_CXX_STANDARD CMAKE_CXX_EXTENSIONS CMAKE_TRY_COMPILE_TARGET_TYPE)
        if ("${BOOST_OPENMETHOD_SAVED_${var}}" STREQUAL "")
            unset(${var})
        else()
            set(${var} "${BOOST_OPENMETHOD_SAVED_${var}}")
        endif()
    endforeach()

    if (NOT BOOST_OPENMETHOD_HAS_REFLECTION)
        message(
            FATAL_ERROR
            "BOOST_OPENMETHOD_ENABLE_REFLECTION is ON but ${CMAKE_CXX_COMPILER_ID} "
            "${CMAKE_CXX_COMPILER_VERSION} does not support C++26 reflection")
    endif()

    message(
        STATUS
        "Boost.OpenMethod: C++26 reflection enabled"
        " [${BOOST_OPENMETHOD_REFLECTION_OPTIONS}]")

    # The standard flag is passed here rather than through CXX_STANDARD: CMake
    # learned the value 26 only in 3.30, and this project supports older ones.
    # Directory compile options come after the flag CMake derives from the
    # library's cxx_std_17 requirement, and the last -std wins. The definition
    # makes a target that does not end up with reflection fail to build, instead
    # of quietly falling back to explicit class registration: see
    # test/test_classes.hpp.
    add_compile_options(${BOOST_OPENMETHOD_REFLECTION_OPTIONS})
    add_compile_definitions(BOOST_OPENMETHOD_EXPECT_REFLECTION)
endif()

if (BOOST_OPENMETHOD_BUILD_EXAMPLES AND NOT BOOST_OPENMETHOD_BUILD_TESTS)
    message(
        WARNING
        "BOOST_OPENMETHOD_BUILD_EXAMPLES requires BOOST_OPENMETHOD_BUILD_TESTS. Examples will not be built.")
    set(BOOST_OPENMETHOD_BUILD_EXAMPLES OFF)
endif()

# Check if environment variable BOOST_SRC_DIR is set
if (NOT DEFINED BOOST_SRC_DIR AND DEFINED ENV{BOOST_SRC_DIR})
    set(DEFAULT_BOOST_SRC_DIR "$ENV{BOOST_SRC_DIR}")
else ()
    set(DEFAULT_BOOST_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")
endif ()
set(BOOST_SRC_DIR ${DEFAULT_BOOST_SRC_DIR} CACHE STRING "Boost source dir to use when running CMake from this directory")

#-------------------------------------------------
#
# Boost modules
#
#-------------------------------------------------
# The boost super-project requires one explicit dependency per-line.

set(
    BOOST_OPENMETHOD_DEPENDENCIES
    Boost::assert
    Boost::config
    Boost::core
    Boost::dynamic_bitset
    Boost::mp11
    Boost::preprocessor
)

if (BOOST_OPENMETHOD_BUILD_TESTS OR BOOST_OPENMETHOD_MRDOCS_BUILD)
    list(APPEND BOOST_OPENMETHOD_DEPENDENCIES Boost::smart_ptr)
    list(APPEND BOOST_OPENMETHOD_DEPENDENCIES Boost::any)
    list(APPEND BOOST_OPENMETHOD_DEPENDENCIES Boost::type_erasure)
    # ... but for its headers only. Boost.TypeErasure's compiled part is one
    # file, dynamic_binding.cpp, which nothing here references and which links
    # Boost::thread - so linking Boost::type_erasure builds Boost.Thread and,
    # through it, Boost.Container, Boost.Chrono, Boost.date_time and
    # Boost.Atomic. Two of those do not build under -Werror on the Windows CI
    # toolchains, which took the whole test suite down with them. Consume the
    # usage requirements without linking, as test/Jamfile does with <use>.
    # ($<COMPILE_ONLY:> expresses this directly, but needs CMake 3.27.)
    list(APPEND BOOST_OPENMETHOD_COMPILE_ONLY_DEPENDENCIES Boost::type_erasure)
endif()

foreach (BOOST_OPENMETHOD_DEPENDENCY ${BOOST_OPENMETHOD_DEPENDENCIES})
    if (BOOST_OPENMETHOD_DEPENDENCY MATCHES "^[ ]*Boost::([A-Za-z0-9_]+)[ ]*$")
        list(APPEND BOOST_OPENMETHOD_INCLUDE_LIBRARIES ${CMAKE_MATCH_1})
    endif ()
endforeach ()

# Conditional dependencies
if (BOOST_OPENMETHOD_MRDOCS_BUILD)
    list(APPEND BOOST_OPENMETHOD_INCLUDE_LIBRARIES smart_ptr)
endif()

if (NOT BOOST_OPENMETHOD_MRDOCS_BUILD)
    if (BOOST_OPENMETHOD_BUILD_TESTS)
        set(BOOST_OPENMETHOD_UNIT_TEST_LIBRARIES test)
        # test/dynamic_loading needs Boost::dll regardless of whether the doc
        # examples are built too, or its tests are silently configured out.
        set(BOOST_OPENMETHOD_EXAMPLE_LIBRARIES dll)
    endif()
endif()

# Complete dependency list
set(BOOST_INCLUDE_LIBRARIES ${BOOST_OPENMETHOD_INCLUDE_LIBRARIES} ${BOOST_OPENMETHOD_UNIT_TEST_LIBRARIES} ${BOOST_OPENMETHOD_EXAMPLE_LIBRARIES})
set(BOOST_EXCLUDE_LIBRARIES openmethod)

#-------------------------------------------------
#
# Add Boost Subdirectory
#
#-------------------------------------------------
if (BOOST_OPENMETHOD_IS_ROOT)
    set(CMAKE_FOLDER Dependencies)
    # Find absolute BOOST_SRC_DIR
    if (NOT IS_ABSOLUTE ${BOOST_SRC_DIR})
        set(BOOST_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${BOOST_SRC_DIR}")
    endif ()

    # Validate BOOST_SRC_DIR
    set(BOOST_SRC_DIR_IS_VALID ON)
    foreach (F "CMakeLists.txt" "Jamroot" "boost-build.jam" "bootstrap.sh" "libs")
        if (NOT EXISTS "${BOOST_SRC_DIR}/${F}")
            message(STATUS "${BOOST_SRC_DIR}/${F} does not exist. Fallback to find_package.")
            set(BOOST_SRC_DIR_IS_VALID OFF)
            break()
        endif ()
    endforeach ()

    # Create Boost interface targets
    if (BOOST_SRC_DIR_IS_VALID)
        # From BOOST_SRC_DIR
        if (BUILD_SHARED_LIBS)
            set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
        endif()
        set(BOOST_EXCLUDE_LIBRARIES ${PROJECT_NAME})
        set(PREV_BUILD_TESTING ${BUILD_TESTING})
        set(BUILD_TESTING OFF CACHE BOOL "Build the tests." FORCE)
        add_subdirectory(${BOOST_SRC_DIR} Dependencies/boost EXCLUDE_FROM_ALL)
        set(BUILD_TESTING ${PREV_BUILD_TESTING} CACHE BOOL "Build the tests." FORCE)
        set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${BOOST_SRC_DIR}/tools/cmake/include")
    else ()
        # From Boost Package
        find_package(Boost REQUIRED COMPONENTS container)
        foreach (BOOST_INCLUDE_LIBRARY ${BOOST_INCLUDE_LIBRARIES})
            if (NOT TARGET Boost::${BOOST_INCLUDE_LIBRARY})
                add_library(Boost::${BOOST_INCLUDE_LIBRARY} ALIAS Boost::headers)
            endif ()
        endforeach ()
    endif ()
    unset(CMAKE_FOLDER)
endif ()

#-------------------------------------------------
#
# Library
#
#-------------------------------------------------
file(GLOB_RECURSE BOOST_OPENMETHOD_HEADERS CONFIGURE_DEPENDS include/boost/*.hpp)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include/boost PREFIX "" FILES ${BOOST_OPENMETHOD_HEADERS})

function(boost_openmethod_setup_properties target)
    set(link_dependencies ${BOOST_OPENMETHOD_DEPENDENCIES})

    # $<COMPILE_ONLY:> is exactly "the usage requirements, without the link".
    # Being a generator expression, it does not care whether the dependency's
    # target has been defined yet - which the super-project, driving this from
    # BOOST_INCLUDE_LIBRARIES, does not guarantee. Anything that inspects the
    # target at configure time does care, and breaks when the super-project
    # happens to configure openmethod first.
    #
    # It needs CMake 3.27, well past the floor declared here. Below that, link
    # normally: slower, and it builds Boost.Thread and friends, but correct.
    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.27)
        foreach (dependency ${BOOST_OPENMETHOD_COMPILE_ONLY_DEPENDENCIES})
            list(REMOVE_ITEM link_dependencies ${dependency})
            list(APPEND link_dependencies $<COMPILE_ONLY:${dependency}>)
        endforeach()
    endif()

    target_link_libraries(${target} INTERFACE ${link_dependencies})
endfunction()

add_library(boost_openmethod INTERFACE)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
  target_sources(boost_openmethod PRIVATE ${BOOST_OPENMETHOD_HEADERS})
endif()

add_library(Boost::openmethod ALIAS boost_openmethod)
target_include_directories(boost_openmethod INTERFACE "${PROJECT_SOURCE_DIR}/include")
boost_openmethod_setup_properties(boost_openmethod)
target_compile_features(boost_openmethod INTERFACE cxx_std_17)

if (BOOST_OPENMETHOD_MRDOCS_BUILD)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/mrdocs.cpp" "#include <boost/openmethod.hpp>\n")
    foreach (HEADER ${BOOST_OPENMETHOD_HEADERS})
        string(REPLACE "${PROJECT_SOURCE_DIR}/include/" "" HEADER "${HEADER}")
        file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/mrdocs.cpp" "#include <${HEADER}>\n")
    endforeach()
    add_library(boost_openmethod_mrdocs "${CMAKE_CURRENT_BINARY_DIR}/mrdocs.cpp")
    target_include_directories(boost_openmethod_mrdocs PUBLIC "${PROJECT_SOURCE_DIR}/include")
    boost_openmethod_setup_properties(boost_openmethod_mrdocs)
    target_link_libraries(boost_openmethod_mrdocs PUBLIC Boost::openmethod)
    target_compile_definitions(boost_openmethod_mrdocs PUBLIC BOOST_OPENMETHOD_MRDOCS)
    return()
endif()

#-------------------------------------------------
#
# Tests
#
#-------------------------------------------------
if (BOOST_OPENMETHOD_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
    if (BOOST_OPENMETHOD_IS_ROOT)
        add_custom_target(all_with_tests ALL DEPENDS tests)
    endif()

    # Examples
    if (BOOST_OPENMETHOD_BUILD_EXAMPLES)
        add_subdirectory(doc/modules/ROOT/examples)
        # Sources behind the `include:` markers in the reference doc comments.
        add_subdirectory(doc/modules/ROOT/snippets)
        # Examples published on Compiler Explorer, see ce/README.md.
        add_subdirectory(ce)
    endif ()
endif ()
