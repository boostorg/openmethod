// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_REGISTRY_HPP
#define BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_REGISTRY_HPP

#include <boost/openmethod/preamble.hpp>

#if defined(_WIN32)
#if defined(EXPORT_REGISTRY)
#define REGISTRY_DECLSPEC boost::openmethod::dllexport
#else
#define REGISTRY_DECLSPEC boost::openmethod::dllimport
#endif

namespace boost::openmethod {
REGISTRY_DECLSPEC
boost_openmethod_declspec(default_registry&);
} // namespace boost::openmethod

#endif

#include <boost/openmethod/default_registry.hpp>

static auto get_ids() -> const void** {
    using namespace boost::openmethod;
    namespace mp11 = boost::mp11;

    constexpr auto n_policies = mp11::mp_size<default_registry::policy_list>::value;
    static const void* ids[1 + n_policies + 1] = {default_registry::id()};
    std::size_t i = 1;

    mp11::mp_for_each<default_registry::policy_list>([&](auto p) {
        using P = decltype(p);

        if constexpr (detail::has_id<default_registry::policy<P>>) {
            ids[i++] = default_registry::policy<P>::id();
        }
    });

    return ids;
}

#endif
