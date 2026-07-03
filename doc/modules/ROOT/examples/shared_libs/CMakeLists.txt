# Copyright (c) 2018-2024 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

message(STATUS "Boost.OpenMethod: building shared library examples")

add_compile_definitions(BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS)

# ------------------------------------------------------------------------------
# static linking

# add_library(boost_openmethod-shared SHARED extensions.cpp)
# target_link_libraries(boost_openmethod-shared Boost::openmethod)
# set_target_properties(boost_openmethod-shared PROPERTIES
#     ENABLE_EXPORTS ON
#     OUTPUT_NAME shared
# )

# add_executable(boost_openmethod-static static_main.cpp)
# target_link_libraries(boost_openmethod-static Boost::openmethod Boost::dll boost_openmethod-shared)
# add_test(NAME boost_openmethod-static COMMAND boost_openmethod-static)

# ------------------------------------------------------------------------------
# dynamic loading, direct virtual_ptrs

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

# NOTE: build the following target (or ALL) when working on this. Just building
# boost_openmethod-dynamic will *not* build the DLL.
add_custom_target(boost_openmethod-shared-all ALL
    DEPENDS boost_openmethod-dynamic boost_openmethod-shared
)

# ------------------------------------------------------------------------------
# dynamic loading, indirect virtual_ptrs

# add_library(boost_openmethod-indirect_shared SHARED indirect_extensions.cpp)
# target_compile_definitions(
#     boost_openmethod-indirect_shared PUBLIC BOOST_OPENMETHOD_DEFAULT_REGISTRY=indirect_registry)
# target_link_libraries(boost_openmethod-indirect_shared PRIVATE Boost::openmethod Boost::dll)
# set_target_properties(boost_openmethod-indirect_shared PROPERTIES
#     ENABLE_EXPORTS ON
#     OUTPUT_NAME indirect_shared
# )

# add_executable(boost_openmethod-indirect indirect_main.cpp)
# target_compile_definitions(
#     boost_openmethod-indirect PUBLIC BOOST_OPENMETHOD_DEFAULT_REGISTRY=indirect_registry)
# set_target_properties(boost_openmethod-indirect PROPERTIES ENABLE_EXPORTS ON)
# target_link_libraries(boost_openmethod-indirect PRIVATE Boost::openmethod Boost::dll)
# add_dependencies(boost_openmethod-indirect boost_openmethod-indirect_shared)
# add_test(NAME boost_openmethod-indirect COMMAND boost_openmethod-indirect)
