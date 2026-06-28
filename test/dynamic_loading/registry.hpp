// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_REGISTRY_HPP
#define BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_REGISTRY_HPP

#include <boost/openmethod/preamble.hpp>

#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(EXPORT_REGISTRY)
#define BOOST_OPENMETHOD_EXPORT_DEFAULT_REGISTRY
#else
#define BOOST_OPENMETHOD_IMPORT_DEFAULT_REGISTRY
#endif
#endif

#include <boost/openmethod/default_registry.hpp>

// The registry under test. The build selects the variant by defining
// BOOST_OPENMETHOD_DEFAULT_REGISTRY on the command line (e.g. to
// ::boost::openmethod::indirect_registry); otherwise fall back to
// default_registry, matching core.hpp's own default. This must be set before
// the .cpp files include core.hpp.
#ifndef BOOST_OPENMETHOD_DEFAULT_REGISTRY
#define BOOST_OPENMETHOD_DEFAULT_REGISTRY ::boost::openmethod::default_registry
#endif

using test_registry = BOOST_OPENMETHOD_DEFAULT_REGISTRY;

static auto get_ids() -> const void** {
    using namespace boost::openmethod;
    namespace mp11 = boost::mp11;

    constexpr auto n_policies = mp11::mp_size<test_registry::policy_list>::value;
    static const void* ids[1 + n_policies + 1] = {test_registry::id()};
    std::size_t i = 1;

    mp11::mp_for_each<test_registry::policy_list>([&](auto p) {
        using P = decltype(p);

        if constexpr (detail::has_id<test_registry::policy<P>>) {
            ids[i++] = test_registry::policy<P>::id();
        }
    });

    return ids;
}

#endif
