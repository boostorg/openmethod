// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_DETAIL_TYPE_KEY_HPP
#define BOOST_OPENMETHOD_DETAIL_TYPE_KEY_HPP

#include <cstddef>
#include <functional>
#include <string_view>

namespace boost::openmethod::detail {

// A key that identifies a class across modules, for an rtti policy that
// identifies classes by name: the name, and for a class that cannot be shared
// across translation units - one with internal linkage, or none - the type_id
// as well, which keeps it apart from other classes with the same name.
struct type_key {
    std::string_view name;
    const void* local;

    friend auto operator==(const type_key& a, const type_key& b) -> bool {
        return a.local == b.local && a.name == b.name;
    }

    friend auto operator!=(const type_key& a, const type_key& b) -> bool {
        return !(a == b);
    }

    friend auto operator<(const type_key& a, const type_key& b) -> bool {
        if (a.name != b.name) {
            return a.name < b.name;
        }

        return std::less<const void*>()(a.local, b.local);
    }
};

} // namespace boost::openmethod::detail

template<>
struct std::hash<boost::openmethod::detail::type_key> {
    auto operator()(const boost::openmethod::detail::type_key& key) const
        -> std::size_t {
        return std::hash<std::string_view>()(key.name) ^
            std::hash<const void*>()(key.local);
    }
};

#endif
