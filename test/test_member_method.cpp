// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>

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
// friend - the motivating case, mirroring the friends.adoc Payroll example.
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

BOOST_OPENMETHOD(
    pay, (Employee & payroll, virtual_ptr<const Employee>), double);

class Payroll : public Employee {
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
        pay, (Employee & payroll, virtual_ptr<const Employee>), double) {
        static_cast<Payroll&>(payroll).update_balance(-5000.0);
        return 5000.0;
    }

    BOOST_OPENMETHOD_DECLARE_OVERRIDER_MEM(
        pay, (Employee & payroll, virtual_ptr<const Salesman> emp), double);
};

BOOST_OPENMETHOD_DEFINE_OVERRIDER_MEM(
    Payroll, pay, (Employee & payroll, virtual_ptr<const Salesman> emp),
    double) {
    // Self-referencing key: names *this* overrider, not the one it calls.
    using self_key = BOOST_OPENMETHOD_OVERRIDER_MEM(
        Payroll, pay, (Employee&, virtual_ptr<const Salesman>), double);
    double base = self_key::method_type::next<self_key::fn>(payroll, emp);
    double commission = emp->sales * 0.05;
    static_cast<Payroll&>(payroll).update_balance(-commission);
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

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
