# Copyright (c) 2018-2024 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

message(STATUS "Boost.OpenMethod: building shared library examples")

add_compile_definitions(BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS)

# ------------------------------------------------------------------------------
# static linking

# The shared library owns the registry state (static_extensions.cpp defines
# BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY); the executable imports it. This
# keeps the link dependency in the usual direction on Windows: the exe links
# the library's import library.
add_library(boost_openmethod-static_shared SHARED static_extensions.cpp)
target_link_libraries(boost_openmethod-static_shared PRIVATE Boost::openmethod)

add_executable(boost_openmethod-static static_main.cpp)
target_link_libraries(boost_openmethod-static
    PRIVATE Boost::openmethod boost_openmethod-static_shared)
# Co-locate the library with the exe so it is found at run time on Windows,
# where there is no rpath.
set_target_properties(boost_openmethod-static_shared PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:boost_openmethod-static>
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:boost_openmethod-static>)

add_test(NAME boost_openmethod-static COMMAND boost_openmethod-static)

# ------------------------------------------------------------------------------
# dynamic loading, direct virtual_ptrs

# The executable owns the registry state (dynamic_main.cpp defines
# BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY); the runtime-loaded library imports
# it, so it links against the exe - the reverse of the usual direction. On
# Windows this needs the exe's import library, generated because of
# ENABLE_EXPORTS; on other platforms ENABLE_EXPORTS adds -rdynamic (or
# equivalent).
add_executable(boost_openmethod-dynamic dynamic_main.cpp)
set_target_properties(boost_openmethod-dynamic PROPERTIES ENABLE_EXPORTS ON)
target_link_libraries(boost_openmethod-dynamic Boost::openmethod Boost::dll)

add_library(boost_openmethod-shared SHARED extensions.cpp)
target_link_libraries(boost_openmethod-shared PRIVATE Boost::openmethod boost_openmethod-dynamic)
# Co-locate the library with the exe on all platforms. On Windows a DLL is a
# runtime output and already lands next to the exe, but on other platforms a
# shared library is a *library* output and would stay in this subdirectory,
# where the test below cannot find it.
set_target_properties(boost_openmethod-shared PROPERTIES
    ENABLE_EXPORTS ON
    LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:boost_openmethod-dynamic>)

# Run from the directory holding the freshly-built exe and its co-located DLL
# (CMAKE_RUNTIME_OUTPUT_DIRECTORY). dynamic_main.cpp loads the DLL by relative
# name, which boost::dll resolves against the working directory; without this
# the test would pick up a stale copy from the source/binary tree.
add_test(
    NAME boost_openmethod-dynamic_shared
    COMMAND boost_openmethod-dynamic
    WORKING_DIRECTORY $<TARGET_FILE_DIR:boost_openmethod-dynamic>)

# ------------------------------------------------------------------------------
# dynamic loading, indirect virtual_ptrs

# Same layout as the previous example, with every target compiled with
# indirect_registry as the default registry. indirect_main.cpp and
# indirect_extensions.cpp use the BOOST_OPENMETHOD_{EXPORT,IMPORT}_INDIRECT_REGISTRY
# macro pair accordingly.
add_executable(boost_openmethod-indirect indirect_main.cpp)
target_compile_definitions(boost_openmethod-indirect
    PRIVATE BOOST_OPENMETHOD_DEFAULT_REGISTRY=indirect_registry)
set_target_properties(boost_openmethod-indirect PROPERTIES ENABLE_EXPORTS ON)
target_link_libraries(boost_openmethod-indirect PRIVATE Boost::openmethod Boost::dll)

add_library(boost_openmethod-indirect_shared SHARED indirect_extensions.cpp)
target_compile_definitions(boost_openmethod-indirect_shared
    PRIVATE BOOST_OPENMETHOD_DEFAULT_REGISTRY=indirect_registry)
target_link_libraries(boost_openmethod-indirect_shared
    PRIVATE Boost::openmethod boost_openmethod-indirect)
set_target_properties(boost_openmethod-indirect_shared PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:boost_openmethod-indirect>
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:boost_openmethod-indirect>)

add_test(
    NAME boost_openmethod-indirect
    COMMAND boost_openmethod-indirect
    WORKING_DIRECTORY $<TARGET_FILE_DIR:boost_openmethod-indirect>)

# NOTE: build the following target (or ALL) when working on this. Just building
# the executables will *not* build the runtime-loaded libraries.
add_custom_target(boost_openmethod-shared-all ALL
    DEPENDS
        boost_openmethod-static boost_openmethod-static_shared
        boost_openmethod-dynamic boost_openmethod-shared
        boost_openmethod-indirect boost_openmethod-indirect_shared
)
