# Copyright (c) 2017-2026 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

message(STATUS "Boost.OpenMethod: building Compiler Explorer examples")

file(GLOB cpp_files "*.cpp")

# reflection.cpp registers its classes by reflection, and has no fallback:
# BOOST_OPENMETHOD_REGISTER_CLASSES expands to nothing without C++26 reflection,
# and no class is registered at all.
if (NOT BOOST_OPENMETHOD_ENABLE_REFLECTION)
    list(REMOVE_ITEM cpp_files "${CMAKE_CURRENT_SOURCE_DIR}/reflection.cpp")
endif()

foreach (cpp ${cpp_files})
    get_filename_component(stem ${cpp} NAME_WE)
    set(test_target "boost_openmethod-ce-${stem}")
    add_executable(${test_target} ${cpp})
    target_link_libraries(${test_target} PRIVATE Boost::openmethod)
    add_test(NAME ${test_target} COMMAND ${test_target})
    add_dependencies(tests ${test_target})
endforeach()
