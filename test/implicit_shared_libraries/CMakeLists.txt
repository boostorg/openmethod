# Copyright (c) 2017-2026 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

# Tests for shared libraries linked *implicitly*, i.e. at build time, as
# opposed to the dlopen'd libraries covered by test/dynamic_loading. All the
# modules are loaded and their static constructors have run before main starts,
# so a single initialize() call sets up dispatch tables holding every module's
# classes, methods and overriders.
#
# In both variants the shared library owns and exports the registry state and
# the executable imports it. That keeps the link dependency in the usual
# direction (the exe links the library), which is what implicit linking needs on
# Windows, and is also the only direction b2 can express.
#
# No BUILD_SHARED_LIBS guard: the libraries are declared explicitly SHARED, so
# they build in a static configuration too.

message(STATUS "Boost.OpenMethod: building implicit shared library tests")

add_subdirectory(default_registry)
add_subdirectory(custom_registry)
add_subdirectory(member_overrider)
