# Copyright (c) 2017-2026 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

# A member method and a member overrider declared in a header that both the
# executable and the shared library include: the two copies of the overrider
# must be consolidated into one, not left ambiguous.

set(lib boost_openmethod-isl_member_overrider_lib)
set(exe boost_openmethod-test_implicit_shared_libraries_member_overrider)

# lib.cpp emits the one definition of the registry state: this library owns
# it, and the executable imports it.
add_library(${lib} SHARED lib.cpp)
target_link_libraries(${lib} PRIVATE Boost::openmethod)

add_executable(${exe} main.cpp)
target_link_libraries(${exe}
    PRIVATE Boost::openmethod Boost::unit_test_framework ${lib})

# Co-locate the library with the executable so it is found at run time on
# Windows, where there is no rpath. On ELF and Mach-O, CMake's default
# build-tree RPATH already resolves an implicitly linked library, but a shared
# library is a *library* output there and would otherwise stay in this
# subdirectory.
set_target_properties(${lib} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${exe}>
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:${exe}>)

# Hidden visibility, which is what the Boost super-project builds with. It is
# the setting that makes this test worth having: at default visibility the
# registrar - an inline variable template - is merged across the two modules by
# the dynamic linker, only one registration survives, and nothing reaches the
# consolidation path at all. Hidden, each module keeps its own copy and
# initialize() has to recognise them as copies of one overrider. The registry
# state is shared through the EXPORT/IMPORT macros, which exist for exactly
# this configuration.
if (NOT WIN32)
    foreach(t ${lib} ${exe})
        target_compile_options(${t} PRIVATE
            -fvisibility=hidden -fvisibility-inlines-hidden)
    endforeach()
endif()

boost_openmethod_add_test(${exe})

if (TARGET tests)
    add_dependencies(tests ${exe} ${lib})
endif()
