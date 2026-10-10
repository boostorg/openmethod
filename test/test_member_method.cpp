// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <utility>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE member_method
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

// ----------------------------------------------------------------------------
// BOOST_OPENMETHOD_MEM: a member method, overloaded, and BOOST_OPENMETHOD_TYPE_MEM.

namespace member_method {

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};
struct Cat : Animal {};

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog, Cat);

struct Zoo {
    BOOST_OPENMETHOD_MEM(poke, (virtual_ptr<Animal>), std::string);
    BOOST_OPENMETHOD_MEM(poke, (virtual_ptr<Animal>, int times), std::string);
};

static_assert(!std::is_same_v<
              BOOST_OPENMETHOD_TYPE_MEM(
                  Zoo::poke, (virtual_ptr<Animal>), std::string),
              BOOST_OPENMETHOD_TYPE_MEM(
                  Zoo::poke, (virtual_ptr<Animal>, int), std::string)>);

// Overriders of the member method, in a class of their own - free-standing
// static member functions, no receiver, no friend needed here since nothing
// private is touched. Both overloads of Zoo::poke are overridden.
class ZooKeeper {
    BOOST_OPENMETHOD_OVERRIDE_MEM(
        Zoo::poke, (virtual_ptr<Dog> d), std::string) {
        (void)d;
        return "one bark";
    }
    BOOST_OPENMETHOD_OVERRIDE_MEM(
        Zoo::poke, (virtual_ptr<Dog> d, int n), std::string) {
        (void)d;
        std::string result;
        for (int i = 0; i < n; ++i) {
            result += "bark ";
        }
        return result;
    }

  public:
    // Public, so the explicit-lookup test below can find it.
    BOOST_OPENMETHOD_OVERRIDE_MEM(Zoo::poke, (virtual_ptr<Cat>), std::string) {
        return "one meow";
    }
};

using poke_cat_key = BOOST_OPENMETHOD_OVERRIDER_MEM(
    ZooKeeper, Zoo::poke, (virtual_ptr<Cat>), std::string);

} // namespace member_method

BOOST_AUTO_TEST_CASE(member_method_call_and_overload) {
    initialize();

    using namespace member_method;

    Dog snoopy;
    Cat felix;

    BOOST_TEST(Zoo::poke(snoopy) == "one bark");
    BOOST_TEST(Zoo::poke(snoopy, 3) == "bark bark bark ");
    BOOST_TEST(Zoo::poke(felix) == "one meow");

    // explicit call, no dispatch
    BOOST_TEST(poke_cat_key::fn(virtual_ptr<Cat>(felix)) == "one meow");
    BOOST_TEST(!poke_cat_key::method_type::has_next<poke_cat_key::fn>());
}

// ----------------------------------------------------------------------------
// Member overriders targeting a FREE method, with private access and no
// friend - the motivating case, mirroring the privacy.adoc Payroll example.
// Also exercises the DECLARE/DEFINE split and next<>/has_next<> through the
// core API from inside a _MEM body (self-referencing key).

namespace member_overrider {

struct Employee {
    virtual ~Employee() = default;
};

struct Salesman : Employee {
    double sales = 0.0;
};

BOOST_OPENMETHOD_TEST_CLASSES(Employee, Salesman);

// Only a reference to it appears in the method's parameter list, so the
// forward declaration is enough - Payroll is not an Employee, and is not
// dispatched on.
class Payroll;

BOOST_OPENMETHOD(pay, (Payroll & payroll, virtual_ptr<const Employee>), double);

class Payroll {
  public:
    double balance() const {
        return balance_;
    }

  private:
    double balance_ = 1'000'000.0;

    void update_balance(double amount) {
        // Private, reachable only because the overriders below are members.
        balance_ += amount;
    }

    BOOST_OPENMETHOD_OVERRIDE_MEM(
        pay, (Payroll & payroll, virtual_ptr<const Employee>), double) {
        payroll.update_balance(-5000.0);
        return 5000.0;
    }

    BOOST_OPENMETHOD_DECLARE_OVERRIDER_MEM(
        pay, (Payroll & payroll, virtual_ptr<const Salesman> emp), double);
};

BOOST_OPENMETHOD_DEFINE_OVERRIDER_MEM(
    Payroll, pay, (Payroll & payroll, virtual_ptr<const Salesman> emp),
    double) {
    // Self-referencing key: names *this* overrider, not the one it calls.
    using self_key = BOOST_OPENMETHOD_OVERRIDER_MEM(
        Payroll, pay, (Payroll&, virtual_ptr<const Salesman>), double);
    double base = self_key::method_type::next<self_key::fn>(payroll, emp);
    double commission = emp->sales * 0.05;
    payroll.update_balance(-commission);
    return base + commission;
}

} // namespace member_overrider

BOOST_AUTO_TEST_CASE(member_overrider_private_access_and_next) {
    initialize();

    using namespace member_overrider;

    Payroll payroll;
    Employee bill;
    Salesman bob;
    bob.sales = 100'000.0;

    BOOST_TEST(pay(payroll, bill) == 5000.0);
    BOOST_TEST(pay(payroll, bob) == 10000.0);
    BOOST_TEST(payroll.balance() == 985000.0);
}

// ----------------------------------------------------------------------------
// A return type containing a comma. The preprocessor hands it over as several
// macro arguments; they reassemble inside va_args_no_registry's template
// argument list, which is exactly what tells a comma-bearing return type apart
// from a trailing registry. Before va_args_no_registry the free macros used
// mp_back for this, and the _MEM ones pasted __VA_ARGS__ in raw.

namespace comma_return {

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};
struct Cat : Animal {};

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog, Cat);

struct Zoo {
    BOOST_OPENMETHOD_MEM(weigh, (virtual_ptr<Animal>), std::pair<int, int>);
};

class Scale {
    // In-class body.
    BOOST_OPENMETHOD_OVERRIDE_MEM(
        Zoo::weigh, (virtual_ptr<Dog>), std::pair<int, int>) {
        return {1, 2};
    }

  public:
    // DECLARE/DEFINE split, defined at namespace scope below.
    BOOST_OPENMETHOD_DECLARE_OVERRIDER_MEM(
        Zoo::weigh, (virtual_ptr<Cat>), std::pair<int, int>);
};

BOOST_OPENMETHOD_DEFINE_OVERRIDER_MEM(
    Scale, Zoo::weigh, (virtual_ptr<Cat>), std::pair<int, int>) {
    return {3, 4};
}

// Naming the method, and naming an overrider, both with a comma in the return
// type - and they agree on the method.
using weigh_method = BOOST_OPENMETHOD_TYPE_MEM(
    Zoo::weigh, (virtual_ptr<Animal>), std::pair<int, int>);
using weigh_cat = BOOST_OPENMETHOD_OVERRIDER_MEM(
    Scale, Zoo::weigh, (virtual_ptr<Cat>), std::pair<int, int>);
static_assert(std::is_same_v<weigh_method, weigh_cat::method_type>);

} // namespace comma_return

// A dependent return type: the macros must not require `typename` from the
// caller. va_args_no_registry's return_type is a nested typedef, so the macro
// supplies it - 459eb0e briefly did not, and this shape stopped compiling.
namespace dependent_return {

using namespace comma_return;

template<class R>
struct Probe {
    using key = BOOST_OPENMETHOD_OVERRIDER_MEM(
        Scale, Zoo::weigh, (virtual_ptr<Cat>), R);
};

static_assert(std::is_same_v<
              Probe<std::pair<int, int>>::key,
              BOOST_OPENMETHOD_OVERRIDER_MEM(
                  Scale, Zoo::weigh, (virtual_ptr<Cat>), std::pair<int, int>)>);

} // namespace dependent_return

BOOST_AUTO_TEST_CASE(member_method_comma_return_type) {
    initialize();

    using namespace comma_return;

    Dog snoopy;
    Cat felix;

    BOOST_TEST((Zoo::weigh(snoopy) == std::pair<int, int>{1, 2}));
    BOOST_TEST((Zoo::weigh(felix) == std::pair<int, int>{3, 4}));

    // explicit call through the overrider key, no dispatch
    BOOST_TEST(
        (weigh_cat::fn(virtual_ptr<Cat>(felix)) == std::pair<int, int>{3, 4}));
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
