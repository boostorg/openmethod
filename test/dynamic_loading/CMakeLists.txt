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
# executable. Each variant compiles the very same sources but with two knobs:
#
#   owner  - which module owns and exports the single shared registry-state
#            symbol: "dll" (lib_registry owns it; method/overrider/exe import it)
#            or "exe" (the executable owns it; all three libraries import it).
#            In the "exe" case the libraries must link *against* the executable
#            (reverse linkage); ENABLE_EXPORTS makes the exe produce the import
#            library / -rdynamic exports needed for that.
#   ARGN   - extra preprocessor definitions (e.g. the default registry) applied
#            to every target so all modules agree (ODR).
#
# Targets are suffixed and outputs go to a per-variant directory so that
# main.cpp, which locates its sibling libraries by filename fragment at runtime,
# never picks up another variant's libraries.
function(boost_openmethod_add_dynamic_loading_variant suffix owner)
    set(extra_defs ${ARGN})
    set(reg  boost_openmethod-dl_test_registry${suffix})
    set(meth boost_openmethod-dl_test_method${suffix})
    set(ovr  boost_openmethod-dl_test_overrider${suffix})
    set(exe  boost_openmethod-test_dynamic_loading${suffix})
    set(outdir "${CMAKE_CURRENT_BINARY_DIR}/dynamic_loading${suffix}")

    add_library(${reg} SHARED registry.cpp)
    add_library(${meth} SHARED method.cpp)
    add_library(${ovr} SHARED overrider.cpp)
    add_executable(${exe} main.cpp)

    # ENABLE_EXPORTS must be set before any library links against the executable
    # (the exe-owned case below), as CMake validates that at the
    # target_link_libraries call. The properties are (re)applied to every target
    # in the foreach further down.
    set_target_properties(${exe} PROPERTIES ENABLE_EXPORTS ON)

    if (owner STREQUAL "exe")
        # The executable owns and exports the registry state; the libraries
        # import it and therefore link against the executable.
        list(APPEND extra_defs REGISTRY_IN_EXE)
        target_link_libraries(${reg}  PUBLIC Boost::openmethod ${exe} PRIVATE Boost::dll)
        target_link_libraries(${meth} PUBLIC Boost::openmethod ${exe} PRIVATE Boost::dll)
        target_link_libraries(${ovr}  PRIVATE Boost::openmethod ${meth} ${exe} Boost::dll)
        target_link_libraries(${exe}
            PRIVATE Boost::openmethod Boost::unit_test_framework Boost::dll)
    else()
        # lib_registry owns and exports the registry state; method/overrider/exe
        # import it. lib_method links lib_registry; the exe links both and loads
        # lib_overrider at runtime.
        target_link_libraries(${reg}  PUBLIC Boost::openmethod PRIVATE Boost::dll)
        target_link_libraries(${meth} PUBLIC Boost::openmethod ${reg} PRIVATE Boost::dll)
        target_link_libraries(${ovr}  PRIVATE Boost::openmethod ${meth} Boost::dll)
        target_link_libraries(${exe}
            PUBLIC Boost::openmethod ${reg} ${meth}
            PRIVATE Boost::openmethod Boost::unit_test_framework Boost::dll)
        # The exe links/loads the libraries, so build them first.
        add_dependencies(${exe} ${reg} ${meth} ${ovr})
    endif()
    # In the exe-owned case the libraries link against the exe, so the build
    # order is the reverse (handled by the link dependency); we must not add
    # exe -> libraries here or we'd form a cycle. Either way, building `tests`
    # builds every module of the variant.

    # Put all of this variant's outputs in one directory so Boost.DLL can locate
    # the shared libraries relative to the test executable at runtime.
    # ENABLE_EXPORTS is required so that symbols from each module are visible to
    # the shared libraries loaded at runtime (e.g. the overrider can resolve
    # symbols from the method/registry/exe, and an exe-owned registry state can
    # be imported by the dlopen'd libraries).
    # We do NOT force CXX_VISIBILITY_PRESET here: the shared registry-state
    # symbol carries BOOST_SYMBOL_VISIBLE in the library (see registry_state in
    # preamble.hpp), so it keeps DEFAULT ELF visibility even under a
    # hidden-visibility build (as set by BoostRoot.cmake in the super-project),
    # and cross-DSO state sharing works without any per-target override.
    foreach(target ${reg} ${meth} ${ovr} ${exe})
        set_target_properties(
            ${target}
            PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${outdir}"
                LIBRARY_OUTPUT_DIRECTORY "${outdir}"
                ARCHIVE_OUTPUT_DIRECTORY "${outdir}"
                ENABLE_EXPORTS ON)
        if (extra_defs)
            target_compile_definitions(${target} PRIVATE ${extra_defs})
        endif()
    endforeach()

    if (TARGET tests)
        add_dependencies(tests ${reg} ${meth} ${ovr} ${exe})
    endif()

    boost_openmethod_add_test(${exe})
endfunction()

# Four variants: {registry owned by dll, by exe} x {default, indirect} registry.
# indirect_registry (default_registry + indirect_vptr) is selected via
# BOOST_OPENMETHOD_DEFAULT_REGISTRY.
boost_openmethod_add_dynamic_loading_variant("_default" dll)
boost_openmethod_add_dynamic_loading_variant(
    "_indirect" dll
    BOOST_OPENMETHOD_DEFAULT_REGISTRY=::boost::openmethod::indirect_registry)
boost_openmethod_add_dynamic_loading_variant("_exereg_default" exe)
boost_openmethod_add_dynamic_loading_variant(
    "_exereg_indirect" exe
    BOOST_OPENMETHOD_DEFAULT_REGISTRY=::boost::openmethod::indirect_registry)
