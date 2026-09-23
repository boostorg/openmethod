# Copyright (c) 2017-2026 Jean-Louis Leroy
# Distributed under the Boost Software License, Version 1.0.
# See accompanying file LICENSE_1_0.txt
# or copy at http://www.boost.org/LICENSE_1_0.txt)

# A member method, declared in a header, used from two translation units.
#
# Everything else in test/ is a single translation unit, which is why this
# shape went uncovered: the _MEM macros declare class members, so a name
# derived from __COUNTER__ makes a class in a header differ between TUs. That
# is an ODR violation no compiler diagnoses, and it is invisible to any test
# built from one .cpp.
#
# No PCH: main.cpp advances __COUNTER__ before including the header, and a
# force-included PCH would be parsed first.

set(exe boost_openmethod-test_cross_tu)

add_executable(${exe} EXCLUDE_FROM_ALL main.cpp other_tu.cpp)
target_link_libraries(${exe}
    PRIVATE Boost::openmethod Boost::unit_test_framework)

boost_openmethod_add_test(${exe})

if (TARGET tests)
    add_dependencies(tests ${exe})
endif()
