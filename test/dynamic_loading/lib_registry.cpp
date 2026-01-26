// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// BOOST_OPENMETHOD_DYNAMIC_LOADING_REGISTRY_EXPORTS is set via CMake compile
// definition, making REGISTRY_API expand to dllexport in registry.hpp (and
// transitively in classes.hpp). Including classes.hpp forces
// static_st<registry_state<...>>::st to be instantiated and exported from
// this shared library, so other libraries can import it.
#include "classes.hpp"
#include <boost/dll/alias.hpp>

using namespace boost::openmethod;
namespace mp11 = boost::mp11;

constexpr auto n_policies = mp11::mp_size<default_registry::policy_list>::value;

static auto get_policy_ids() -> const void** {
    static const void* ids[n_policies + 1];
    static bool init = [] {
        std::size_t i = 0;
        mp11::mp_for_each<default_registry::policy_list>([&](auto p) {
            using P = decltype(p);
            if constexpr (detail::has_id<default_registry::policy<P>>) {
                ids[i++] = default_registry::policy<P>::id();
            }
        });
        ids[i] = nullptr;
        return true;
    }();
    (void)init;
    return ids;
}

BOOST_DLL_ALIAS(get_policy_ids, dl_registry_get_policy_ids)
