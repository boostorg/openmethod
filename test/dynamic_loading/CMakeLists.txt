# Copyright (c) 2018-2025 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

if (BUILD_SHARED_LIBS)
    message(STATUS "Building shared library tests")
else()
    return()
endif()

# Build one set of {registry, method, overrider} shared libraries plus a test
# executable. Each variant compiles the very same sources but with a different
# default registry, selected by extra preprocessor definitions (ARGN) applied to
# every target so all modules agree (ODR). Targets are suffixed and outputs go
# to a per-variant directory so that main.cpp, which locates its sibling
# libraries by filename fragment at runtime, never picks up the other variant.
function(boost_openmethod_add_dynamic_loading_variant suffix)
    set(extra_defs ${ARGN})
    set(reg  boost_openmethod-dl_test_registry${suffix})
    set(meth boost_openmethod-dl_test_method${suffix})
    set(ovr  boost_openmethod-dl_test_overrider${suffix})
    set(exe  boost_openmethod-test_dynamic_loading${suffix})
    set(outdir "${CMAKE_CURRENT_BINARY_DIR}/dynamic_loading${suffix}")

    # lib_registry: exports the registry's static policy state
    add_library(${reg} SHARED registry.cpp)
    target_link_libraries(${reg} PUBLIC Boost::openmethod PRIVATE Boost::dll)

    # lib_method: exports the speak method; imports registry state from lib_registry
    add_library(${meth} SHARED method.cpp)
    target_link_libraries(${meth} PUBLIC Boost::openmethod ${reg} PRIVATE Boost::dll)

    # lib_overrider: adds a Dog overrider; imported at runtime via Boost.DLL
    add_library(${ovr} SHARED overrider.cpp)
    target_link_libraries(${ovr} PRIVATE Boost::openmethod ${meth} Boost::dll)

    # Test executable: links against lib_method (and lib_registry transitively);
    # dynamically loads lib_overrider at runtime.
    add_executable(${exe} main.cpp)
    target_link_libraries(${exe}
        PUBLIC Boost::openmethod ${reg} ${meth}
        PRIVATE Boost::openmethod Boost::unit_test_framework Boost::dll)
    add_dependencies(${exe} ${reg} ${meth} ${ovr})

    # Put all of this variant's outputs in one directory so Boost.DLL can locate
    # the shared libraries relative to the test executable at runtime.
    # ENABLE_EXPORTS is required so that symbols from each shared library are
    # visible to other shared libraries loaded at runtime (e.g. the overrider
    # can resolve symbols from the method/registry libraries).
    # CXX_VISIBILITY_PRESET must be "default" so that the template static
    # variables used as the shared registry state are exported with DEFAULT ELF
    # visibility. With hidden visibility (as set by BoostRoot.cmake in the
    # super-project build) they become UNIQUE HIDDEN and are not deduplicated by
    # the dynamic linker, breaking cross-DSO state sharing.
    foreach(target ${reg} ${meth} ${ovr} ${exe})
        set_target_properties(
            ${target}
            PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${outdir}"
                LIBRARY_OUTPUT_DIRECTORY "${outdir}"
                ARCHIVE_OUTPUT_DIRECTORY "${outdir}"
                ENABLE_EXPORTS ON
                CXX_VISIBILITY_PRESET default
                VISIBILITY_INLINES_HIDDEN OFF)
        if (extra_defs)
            target_compile_definitions(${target} PRIVATE ${extra_defs})
        endif()
    endforeach()

    if (TARGET tests)
        add_dependencies(tests ${exe})
    endif()

    boost_openmethod_add_test(${exe})
endfunction()

# Variant 1: the default registry.
boost_openmethod_add_dynamic_loading_variant("")

# Variant 2: indirect_registry (default_registry + indirect_vptr policy),
# selected via BOOST_OPENMETHOD_DEFAULT_REGISTRY.
boost_openmethod_add_dynamic_loading_variant(
    "_indirect"
    BOOST_OPENMETHOD_DEFAULT_REGISTRY=::boost::openmethod::indirect_registry)
