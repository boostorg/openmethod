#ifndef BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_GET_IDS_HPP
#define BOOST_OPENMETHOD_TEST_DYNAMIC_LOADING_GET_IDS_HPP

template<class Registry>
static auto get_ids() -> const void** {

    constexpr auto n_policies = mp11::mp_size<Registry::policy_list>::value;
    static const void* ids[1 + n_policies + 1] = {Registry::id()};
    std::size_t i = 1;

    mp11::mp_for_each<Registry::policy_list>([&](auto p) {
        using P = decltype(p);

        if constexpr (detail::has_id<Registry::policy<P>>) {
            ids[i++] = Registry::policy<P>::id();
        }
    });

    return ids;
}
