# Copyright (c) 2017-2026 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

# named_static_rtti shared across an implicitly linked library: the library and
# the executable have different type_ids for the same class, and the registry
# merges them by name. See main.cpp.

set(lib boost_openmethod-isl_named_static_rtti_lib)
set(exe boost_openmethod-test_implicit_shared_libraries_named_static_rtti)

add_library(${lib} SHARED lib.cpp)
target_link_libraries(${lib} PRIVATE Boost::openmethod)

add_executable(${exe} main.cpp)
target_link_libraries(${exe}
    PRIVATE Boost::openmethod Boost::unit_test_framework ${lib})

# Hidden visibility is the configuration under test - it is b2's default, and
# what BoostRoot.cmake sets. At default visibility, ELF unifies the static
# variables behind the type_ids across modules, and there is nothing to merge.
# Windows DLLs never share them, whatever the setting.
set_target_properties(${lib} ${exe} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)

# See custom_registry/CMakeLists.txt.
set_target_properties(${lib} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${exe}>
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${exe}>)

boost_openmethod_add_test(${exe})

if (TARGET tests)
    add_dependencies(tests ${exe} ${lib})
endif()
