# Copyright (c) 2017-2026 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

# Sources for the `include:` markers in the reference doc comments; see
# doc/mrdocs-addons/extensions/include.lua. Built and run alongside the
# examples, which is the whole point: a reference example cannot drift from the
# library without this failing.
#
# Targets carry a `snippet_` prefix because a snippet and an example may share
# a stem -- both directories have a virtual_ptr.cpp.

message(STATUS "Boost.OpenMethod: building documentation snippets")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS)
endif()

file(GLOB cpp_files "*.cpp")

foreach (cpp ${cpp_files})
    get_filename_component(stem ${cpp} NAME_WE)
    set(test_target "boost_openmethod-snippet_${stem}")
    add_executable(${test_target} ${cpp})
    target_link_libraries(${test_target} PRIVATE Boost::openmethod Boost::unit_test_framework)
    add_test(NAME ${test_target} COMMAND ${test_target})
    add_dependencies(tests ${test_target})
endforeach()
