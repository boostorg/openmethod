# Copyright (c) 2018-2025 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

if (BUILD_SHARED_LIBS)
    message(STATUS "Building shared library tests")
else()
    message(STATUS "Skipping dynamic_loading tests (BUILD_SHARED_LIBS is OFF)")
    return()
endif()

# Build one set of {registry, method, overrider} shared libraries plus a test
# executable. Each variant compiles the very same sources but with three
# knobs:
#
#   owner            - which module owns and exports the single shared
#                       registry-state symbol: "dll" (lib_registry owns it;
#                       method/overrider/exe import it) or "exe" (the
#                       executable owns it; method/overrider import it).
#                       In the "exe" case the libraries must link *against*
#                       the executable (reverse linkage); ENABLE_EXPORTS makes
#                       the exe produce the import library / -rdynamic exports
#                       needed for that. lib_registry is still built and
#                       link-configured for reverse linkage in this case (for
#                       symmetry, and so `tests` fully builds every module),
#                       but main.cpp only dlopens the method/overrider
#                       fragments (see below) - lib_registry is never loaded
#                       into the running process, so the exe-owned state is
#                       exercised only via lib_method/lib_overrider, not
#                       lib_registry itself.
#   hidden_visibility - if true, force CXX_VISIBILITY_PRESET hidden on every
#                       target. classes.hpp marks Animal/Dog/Cat
#                       BOOST_SYMBOL_VISIBLE, so under this setting their
#                       type_id unifies across modules while each module's
#                       Registry::static_vptr<Class> - not covered by that
#                       attribute - stays a per-module copy: exactly the
#                       configuration the class-dedup fix in
#                       augment_classes() (initialize.hpp) targets.
#   ARGN              - extra preprocessor definitions (e.g. the default
#                       registry) applied to every target so all modules agree
#                       (ODR).
#
# Targets are suffixed and outputs go to a per-variant directory so that
# main.cpp, which locates its sibling libraries by filename fragment at runtime,
# never picks up another variant's libraries.
function(boost_openmethod_add_dynamic_loading_variant suffix owner hidden_visibility)
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
    # symbols from the method/exe, and an exe-owned registry state can be
    # imported by the dlopen'd method/overrider libraries).
    # We do NOT force CXX_VISIBILITY_PRESET here except for the hidden_visibility
    # variant below: the shared registry-state symbol carries BOOST_SYMBOL_VISIBLE
    # in the library (see registry_state in preamble.hpp), so it keeps DEFAULT ELF
    # visibility even under a hidden-visibility build (as set by BoostRoot.cmake in
    # the super-project), and cross-DSO state sharing works without any per-target
    # override. The hidden_visibility variant exists to reproduce, on a standalone
    # (non-super-project) build, the configuration where classes.hpp's
    # BOOST_SYMBOL_VISIBLE classes keep default RTTI visibility while everything
    # else (including Registry::static_vptr<Class>) is hidden per module.
    foreach(target ${reg} ${meth} ${ovr} ${exe})
        set_target_properties(
            ${target}
            PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${outdir}"
                LIBRARY_OUTPUT_DIRECTORY "${outdir}"
                ARCHIVE_OUTPUT_DIRECTORY "${outdir}"
                ENABLE_EXPORTS ON)
        if (hidden_visibility)
            set_target_properties(
                ${target}
                PROPERTIES
                    CXX_VISIBILITY_PRESET hidden
                    VISIBILITY_INLINES_HIDDEN ON)
        endif()
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
boost_openmethod_add_dynamic_loading_variant("_default" dll OFF)
boost_openmethod_add_dynamic_loading_variant(
    "_indirect" dll OFF
    BOOST_OPENMETHOD_DEFAULT_REGISTRY=::boost::openmethod::indirect_registry)
boost_openmethod_add_dynamic_loading_variant("_exereg_default" exe OFF)
boost_openmethod_add_dynamic_loading_variant(
    "_exereg_indirect" exe OFF
    BOOST_OPENMETHOD_DEFAULT_REGISTRY=::boost::openmethod::indirect_registry)

# Fifth variant: forces hidden ELF visibility to reproduce, on a standalone
# build, the class-dedup configuration that augment_classes() must handle
# correctly (see the function comment above and classes.hpp).
boost_openmethod_add_dynamic_loading_variant("_hidden_vis" dll ON)
