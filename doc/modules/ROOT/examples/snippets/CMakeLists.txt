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

# Two translation units that disagree on BOOST_OPENMETHOD_ENABLE_RUNTIME_CHECKS:
# the other one sits in a subdirectory, out of the glob's reach.
target_sources(boost_openmethod-snippet_errors_odr_violation PRIVATE odr_violation/other.cpp)

# minimal_cover_hash needs BMI2 in the translation unit that names it.
# BOOST_OPENMETHOD_HAS_BMI2 is the cached result of test/CMakeLists.txt's probe;
# without it, the snippet compiles to an empty test.
if (BOOST_OPENMETHOD_HAS_BMI2)
    if (MSVC)
        target_compile_options(boost_openmethod-snippet_errors_too_many_bits PRIVATE /arch:AVX2)
    else()
        target_compile_options(boost_openmethod-snippet_errors_too_many_bits PRIVATE -mbmi2)
    endif()
endif()
