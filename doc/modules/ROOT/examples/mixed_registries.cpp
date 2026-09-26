// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#define BOOST_TEST_MODULE mixed_registries
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

// tag::nodes[]
struct Node {
    explicit Node(unsigned type) : type(type) {
    }

    unsigned type;
    static constexpr unsigned static_type = 1;
};

struct Number : Node {
    static constexpr unsigned static_type = 2;

    explicit Number(int value) : Node(static_type), value(value) {
    }

    int value;
};

struct Plus : Node {
    static constexpr unsigned static_type = 3;

    Plus(const Node& left, const Node& right) :
        Node(static_type), left(left), right(right) {
    }

    const Node& left;
    const Node& right;
};

auto next_non_node_type_id() -> type_id {
    static auto next = std::numeric_limits<std::uintptr_t>::max();

    return reinterpret_cast<type_id>(next--);
}

auto non_node_type_index(type_id type) -> std::size_t {
    return std::numeric_limits<std::uintptr_t>::max() -
        reinterpret_cast<std::uintptr_t>(type) + 1;
}

struct node_rtti : policies::rtti {
    template<class Registry>
    struct fn : defaults {
        template<class T>
        static constexpr bool is_polymorphic = std::is_base_of_v<Node, T>;

        template<typename T>
        static auto static_type() -> type_id {
            if constexpr (is_polymorphic<T>) {
                return reinterpret_cast<type_id>(T::static_type);
            } else {
                static const auto id = next_non_node_type_id();

                return id;
            }
        }

        template<typename T>
        static auto dynamic_type(const T& obj) -> type_id {
            if constexpr (is_polymorphic<T>) {
                return reinterpret_cast<type_id>(obj.type);
            } else {
                return nullptr;
            }
        }

        template<class Stream>
        static void type_name(type_id type, Stream& stream) {
            static const char* const names[] = {"Node", "Number", "Plus"};
            auto id = reinterpret_cast<std::uintptr_t>(type);

            if (id >= 1 && id <= 3) {
                stream << names[id - 1];
            } else {
                stream << "<" << non_node_type_index(type) << ">";
            }
        }
    };
};

struct node_registry : registry<node_rtti, policies::vptr_vector> {};

BOOST_OPENMETHOD_CLASSES(Node, Number, Plus, node_registry);
// end::nodes[]

// tag::value[]
BOOST_OPENMETHOD(value, (virtual_<const Node&>), int, node_registry);

BOOST_OPENMETHOD_OVERRIDE(value, (const Number& number), int) {
    return number.value;
}

BOOST_OPENMETHOD_OVERRIDE(value, (const Plus& plus), int) {
    return value(plus.left) + value(plus.right);
}
// end::value[]

// tag::formats[]
struct Format {
    virtual ~Format() = default;
};

struct Postfix : Format {};
struct Infix : Format {};

BOOST_OPENMETHOD_CLASSES(Format, Postfix, Infix);
// end::formats[]

// tag::method[]
BOOST_OPENMETHOD(
    render, (virtual_<const Node&, node_registry>, virtual_<const Format&>),
    std::string, default_registry);

BOOST_OPENMETHOD_OVERRIDE(
    render, (const Number& number, const Format&), std::string) {
    return std::to_string(number.value);
}

BOOST_OPENMETHOD_OVERRIDE(
    render, (const Plus& plus, const Postfix& format), std::string) {
    return render(plus.left, format) + " " + render(plus.right, format) + " +";
}

BOOST_OPENMETHOD_OVERRIDE(
    render, (const Plus& plus, const Infix& format), std::string) {
    return "(" + render(plus.left, format) + " + " +
        render(plus.right, format) + ")";
}
// end::method[]

BOOST_AUTO_TEST_CASE(mixed_registries) {
    // tag::initialize[]
    initialize<node_registry>();
    initialize();
    // end::initialize[]

    std::ostringstream captured;
    auto* cout_buf = std::cout.rdbuf(captured.rdbuf());

    // tag::call[]
    Number one(1), two(2), three(3);
    Plus sum(one, two), total(sum, three);

    std::cout << render(total, Postfix()) << " = " << value(total) << "\n";
    std::cout << render(total, Infix()) << " = " << value(total) << "\n";
    // end::call[]

    std::cout.rdbuf(cout_buf);

    BOOST_TEST(captured.str() == "1 2 + 3 + = 6\n((1 + 2) + 3) = 6\n");
}
