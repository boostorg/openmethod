// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod/default_registry.hpp>

namespace {
template<typename T>
inline char non_polymorphic_static_type_storage = '\0';
} // anonymous namespace

struct Animal {
    const char* name;

    Animal(const char* name, const char* type) : name(name), type(type) {
    }

    static constexpr const char* static_type = "Animal";
    const char* type;
};

struct Dog : Animal {
    Dog(const char* name, const char* type = static_type) : Animal(name, type) {
    }

    static constexpr const char* static_type = "Dog";
};

struct Cat : Animal {
    Cat(const char* name, const char* type = static_type) : Animal(name, type) {
    }

    static constexpr const char* static_type = "Cat";
};

struct custom_rtti : boost::openmethod::policies::rtti {
    template<class Registry>
    struct fn : defaults {
        template<class T>
        static constexpr bool is_polymorphic = std::is_base_of_v<Animal, T>;

        template<typename T>
        static auto static_type() {
            if constexpr (is_polymorphic<T>) {
                return T::static_type;
            } else {
                return static_cast<const char*>(
                    &non_polymorphic_static_type_storage<T>);
            }
        }

        template<typename T>
        static auto dynamic_type(const T& obj) {
            if constexpr (is_polymorphic<T>) {
                return obj.type;
            } else {
                return static_cast<const char*>(
                    &non_polymorphic_static_type_storage<T>);
            }
        }

        template<class Stream>
        static void type_name(
            boost::openmethod::type_id type, Stream& stream) {
            stream
                << (type == nullptr ? "?"
                                    : reinterpret_cast<const char*>(type));
        }

        static auto type_index(boost::openmethod::type_id type) {
            return type;
        }
    };
};

struct test_registry
    : boost::openmethod::default_registry::with<custom_rtti> {};

#define BOOST_OPENMETHOD_DEFAULT_REGISTRY test_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE custom_rtti_simple_projection
#include <boost/test/unit_test.hpp>
#include <boost/utility/identity_type.hpp>

using namespace boost::openmethod;

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);

BOOST_OPENMETHOD(poke, (virtual_<Animal&>, std::ostream&), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (Dog & dog, std::ostream& os), void) {
    os << dog.name << " barks.";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (Cat & cat, std::ostream& os), void) {
    os << cat.name << " hisses.";
}

BOOST_AUTO_TEST_CASE(custom_rtti_simple_projection) {
    initialize();

    Animal &&a = Dog("Snoopy"), &&b = Cat("Sylvester");

    {
        std::stringstream os;
        poke(a, os);
        BOOST_TEST(os.str() == "Snoopy barks.");
    }
    {
        std::stringstream os;
        poke(b, os);
        BOOST_TEST(os.str() == "Sylvester hisses.");
    }
}
