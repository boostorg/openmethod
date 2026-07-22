// Copyright (c) 2018-2025 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/openmethod/default_registry.hpp>

namespace {
constexpr std::size_t non_polymorphic_high_bit = std::size_t(1)
    << (sizeof(std::size_t) * 8 - 1);

inline std::size_t next_non_polymorphic_id() {
    static std::size_t counter = 0;
    return non_polymorphic_high_bit | ++counter;
}

template<typename T>
inline std::size_t non_polymorphic_static_type() {
    static std::size_t value = next_non_polymorphic_id();
    return value;
}
} // anonymous namespace

struct Animal {
    const char* name;

    Animal(const char* name, std::size_t type) : name(name), type(type) {
    }

    static constexpr std::size_t static_type = 1;
    std::size_t type;
};

struct Dog : Animal {
    Dog(const char* name, std::size_t type = static_type) : Animal(name, type) {
    }

    static constexpr std::size_t static_type = 2;
};

struct Cat : Animal {
    Cat(const char* name, std::size_t type = static_type) : Animal(name, type) {
    }

    static constexpr std::size_t static_type = 3;
};

struct custom_rtti : boost::openmethod::policies::rtti {
    template<class Registry>
    struct fn : defaults {
        template<class T>
        static constexpr bool is_polymorphic = std::is_base_of_v<Animal, T>;

        template<typename T>
        static auto static_type() {
            if constexpr (is_polymorphic<T>) {
                return boost::openmethod::type_id(T::static_type);
            } else {
                return boost::openmethod::type_id(
                    non_polymorphic_static_type<T>());
            }
        }

        template<typename T>
        static auto dynamic_type(const T& obj) {
            if constexpr (is_polymorphic<T>) {
                return boost::openmethod::type_id(obj.type);
            } else {
                return boost::openmethod::type_id(
                    non_polymorphic_static_type<T>());
            }
        }

        template<class Stream>
        static void type_name(boost::openmethod::type_id type, Stream& stream) {
            static const char* name[] = {"Animal", "Dog", "Cat"};
            auto idx = reinterpret_cast<std::size_t>(type);
            stream << (idx >= 1 && idx <= 3 ? name[idx - 1] : "?");
        }

        static auto type_index(boost::openmethod::type_id type) {
            return type;
        }
    };
};

struct test_registry
    : boost::openmethod::default_registry::with<custom_rtti>::without<
          boost::openmethod::policies::type_hash> {};

#define BOOST_OPENMETHOD_DEFAULT_REGISTRY test_registry

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE custom_rtti_simple
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

void call_poke(Animal& a, std::ostream& os) {
    return poke(a, os);
}
// mov     rax, qword ptr [rdi + 8]
// mov     rcx, qword ptr [rip + domain<test_registry>::context+24]
// mov     rax, qword ptr [rcx + 8*rax]
// mov     rcx, qword ptr [rip + method<test_registry, poke, void
// (virtual_<Animal&>, basic_ostream<char, char_traits<char> >&)>::fn+80] mov
// rax, qword ptr [rax + 8*rcx] jmp     rax                             #
// TAILCALL

BOOST_AUTO_TEST_CASE(custom_rtti_simple) {
    BOOST_TEST(Animal::static_type == 1u);
    BOOST_TEST(Dog::static_type == 2u);
    BOOST_TEST(Cat::static_type == 3u);
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

namespace using_vptr {

template<class C>
using vptr = virtual_ptr<C, test_registry>;

BOOST_OPENMETHOD(poke, (vptr<Animal>, std::ostream&), void);

BOOST_OPENMETHOD_OVERRIDE(poke, (vptr<Dog> dog, std::ostream& os), void) {
    os << dog->name << " barks.";
}

BOOST_OPENMETHOD_OVERRIDE(poke, (vptr<Cat> cat, std::ostream& os), void) {
    os << cat->name << " hisses.";
}

void call_poke(vptr<Animal> a, std::ostream& os) {
    // mov     rax, qword ptr [rip + method<test_registry, poke, ...>::fn+80]
    // mov     rax, qword ptr [rsi + 8*rax]
    // jmp     rax                             # TAILCALL
    return poke(a, os);
}

} // namespace using_vptr
