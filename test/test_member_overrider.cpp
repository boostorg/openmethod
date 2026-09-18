// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>

#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>

#include "test_util.hpp"

#define BOOST_TEST_MODULE member_overrider
#include <boost/test/unit_test.hpp>

using namespace boost::openmethod;

// ----------------------------------------------------------------------------
// BOOST_OPENMETHOD_OVERRIDE_FN at namespace scope, on already-existing free
// functions - the case it serves independently of member overriders.

namespace free_fn {

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};
struct Cat : Animal {};

BOOST_OPENMETHOD_TEST_CLASSES(Animal, Dog, Cat);

BOOST_OPENMETHOD(speak, (virtual_ptr<const Animal>), std::string);

auto speak_dog(virtual_ptr<const Dog>) -> std::string {
    return "bark";
}

auto speak_cat(virtual_ptr<const Cat>) -> std::string {
    return "meow";
}

BOOST_OPENMETHOD_OVERRIDE_FN(
    speak, (virtual_ptr<const Animal>), std::string, speak_dog, speak_cat);

} // namespace free_fn

// ----------------------------------------------------------------------------
// Member overriders: static member functions of a class other than the one
// dispatched on, given access to its private state with no `friend`
// declaration - the motivating case documented (via the `friend` idiom it
// replaces) in doc/modules/ROOT/pages/friends.adoc.

namespace member {

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
        // A private member, reachable from the overriders below only because
        // they are members of Payroll too - no friend declaration needed.
        balance_ += amount;
    }

    static auto pay_employee(Employee& payroll, virtual_ptr<const Employee>)
        -> double {
        double amount = 5000.0;
        static_cast<Payroll&>(payroll).update_balance(-amount);
        return amount;
    }

    static auto pay_salesman(Employee& payroll, virtual_ptr<const Salesman> emp)
        -> double {
        double base = pay_employee(payroll, emp);
        double commission = emp->sales * 0.05;
        static_cast<Payroll&>(payroll).update_balance(-commission);
        return base + commission;
    }

    // One registrar, naming both member overriders of `pay` for this class -
    // override<Fn...> is already variadic, so this is not one line per
    // overrider.
    BOOST_OPENMETHOD_OVERRIDE_FN(
        pay, (Employee & payroll, virtual_ptr<const Employee>), double,
        &Payroll::pay_employee, &Payroll::pay_salesman);
};

} // namespace member

BOOST_AUTO_TEST_CASE(override_fn_namespace_scope) {
    initialize();

    using namespace free_fn;

    Dog snoopy;
    Cat felix;
    BOOST_TEST(speak(virtual_ptr<const Animal>(snoopy)) == "bark");
    BOOST_TEST(speak(virtual_ptr<const Animal>(felix)) == "meow");
}

BOOST_AUTO_TEST_CASE(member_overrider_private_access) {
    initialize();

    using namespace member;

    Payroll payroll;
    Employee bill;
    Salesman bob;
    bob.sales = 100'000.0;

    BOOST_TEST(pay(payroll, virtual_ptr<const Employee>(bill)) == 5000.0);
    BOOST_TEST(pay(payroll, virtual_ptr<const Employee>(bob)) == 10000.0);
    BOOST_TEST(payroll.balance() == 985000.0);
}

BOOST_OPENMETHOD_TEST_REGISTER_CLASSES();
