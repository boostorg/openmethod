// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_OPENMETHOD_TEST_CROSS_TU_MEMBER_METHOD_HPP
#define BOOST_OPENMETHOD_TEST_CROSS_TU_MEMBER_METHOD_HPP

#include <string>

#include <boost/openmethod.hpp>

using namespace boost::openmethod;

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};
struct Cat : Animal {};

// A member method has nowhere to live but a header, so everything the _MEM
// macros emit must be identical in every translation unit that includes this
// file. They declare class *members*, so a name derived from __COUNTER__ would
// give Zoo and Keeper a different member-specification per TU.
struct Zoo {
    BOOST_OPENMETHOD_MEM(poke, (virtual_ptr<Animal>), std::string);
};

// An overrider that is itself in the header, so both TUs see it. It must
// register exactly once: the registrar is a namespace-scope variable template
// keyed on its own type, hence one entity program-wide.
class Keeper {
    BOOST_OPENMETHOD_OVERRIDE_MEM(Zoo::poke, (virtual_ptr<Cat>), std::string) {
        return "hiss";
    }
};

#endif
