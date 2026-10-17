// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// std_rtti::type_index must be equal for the copies of a class in different
// modules, and different for two classes with the same name in anonymous
// namespaces, or unnamed. Where std::type_index does not do that, it looks for
// their marks in the mangled name.

#include <boost/openmethod.hpp>

#define BOOST_TEST_MODULE openmethod
#include <boost/test/unit_test.hpp>

#include <typeinfo>

using boost::openmethod::default_registry;
using boost::openmethod::detail::itanium_internal_linkage;

// Names produced by GCC, Clang, and Clang with libc++.
BOOST_AUTO_TEST_CASE(mangled_names) {
    const char* shared[] = {
        "6Global",
        "N2ns5InnerE",
        "10ZLibStream",
        "12UtilityClass",
        "8Foo$Impl", // `$` is a valid identifier character on GCC and Clang
        "6A$B$C$",
        "N3app3BoxINS_1XENS_7UtilityEE", // `Ut` after a substitution
        "3BoxISt3mapINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt6"
        "vectorIiSaIiEESt4lessIS6_ESaISt4pairIKS6_S9_EEEE",
        "3BoxINSt3__13mapINS0_12basic_stringIcNS0_11char_traitsIcEENS0_"
        "9allocatorIcEEEENS0_6vectorIiNS5_IiEEEENS0_4lessIS7_EENS5_INS0_"
        "4pairIKS7_SA_EEEEEEE",
        "4VBoxIL5Color12EE",
        "4VBoxILN2ns5ShadeE12EE",
        "4VBoxILin3EE",
        "4VBoxIXadL_Z10extern_varEEE",
        "4VBoxIXadL_Z10extern_varEEL5Color1EE", // an enum literal after `_Z`
        "3BoxIA10_iE",
        "Z15local_in_externvE5Local",
        "N5boost10openmethod6methodI22speak_boost_openmethodFvNS0_11virtual_"
        "ptrI6GlobalNS0_16default_registryEvEESt3mapINSt7__cxx1112basic_"
        "stringIcSt11char_traitsIcESaIcEEESt6vectorIiSaIiEESt4lessISD_ESaISt4"
        "pairIKSD_SG_EEEES5_EE",
    };

    for (auto name : shared) {
        BOOST_TEST_INFO(name);
        BOOST_TEST(!itanium_internal_linkage(name));
    }

    const char* internal[] = {
        "N12_GLOBAL__N_14AnonE",
        "3BoxIN12_GLOBAL__N_14AnonEE",
        "N3app3BoxINS_12_GLOBAL__N_11YEEE",         // after a substitution
        "4VBoxILN2ns5ShadeE12EN12_GLOBAL__N_11XEE", // after an enum literal
        "3$_0",                                     // Clang: an unnamed type
        "10._anon_100",                             // GCC: an unnamed type
    };

    for (auto name : internal) {
        BOOST_TEST_INFO(name);
        BOOST_TEST(itanium_internal_linkage(name));
    }
}

struct Global {};

struct Other {};

namespace {
struct Anon {};
} // namespace

template<class T>
auto key() {
    return default_registry::rtti::type_index(
        default_registry::rtti::static_type<T>());
}

BOOST_AUTO_TEST_CASE(keys) {
    BOOST_TEST((key<Global>() == key<Global>()));
    BOOST_TEST((key<Global>() != key<Other>()));
    BOOST_TEST((key<Global>() < key<Other>() || key<Other>() < key<Global>()));

#if !BOOST_OPENMETHOD_DETAIL_STD_TYPE_INDEX
    // The scanner is in use: check it on the names of this compiler.
    BOOST_TEST(key<Global>().local == nullptr);
    BOOST_TEST(
        key<Anon>().local == default_registry::rtti::static_type<Anon>());
    BOOST_TEST(!itanium_internal_linkage(typeid(Global).name()));
    BOOST_TEST(itanium_internal_linkage(typeid(Anon).name()));
#endif
}
