// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// tag::content[]
#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <iostream>

struct Animal {
    virtual ~Animal() = default;
};

struct Dog : Animal {};
struct Cat : Animal {};

BOOST_OPENMETHOD_CLASSES(Animal, Dog, Cat);

// tag::zoo[]
struct Zoo {
    BOOST_OPENMETHOD_MEM(
        poke, (boost::openmethod::virtual_ptr<Animal>), std::string);
};
// end::zoo[]

// tag::zookeeper[]
class ZooKeeper {
    BOOST_OPENMETHOD_OVERRIDE_MEM(
        Zoo::poke, (boost::openmethod::virtual_ptr<Dog>), std::string) {
        return "bark";
    }
    BOOST_OPENMETHOD_OVERRIDE_MEM(
        Zoo::poke, (boost::openmethod::virtual_ptr<Cat>), std::string) {
        return "hiss";
    }
};
// end::zookeeper[]

class Payroll;

struct Employee {
    virtual ~Employee() = default;
};

struct Salesman : Employee {
    double sales = 0.0;
};

// tag::pay[]
BOOST_OPENMETHOD(
    pay, (Payroll & payroll, boost::openmethod::virtual_ptr<const Employee>),
    double);
// end::pay[]

// tag::payroll[]
class Payroll {
  public:
    double balance() const {
        return balance_;
    }

  private:
    double balance_ = 1'000'000.0;

    void update_balance(double amount) {
        // throw if balance would become negative
        balance_ += amount;
    }

    BOOST_OPENMETHOD_OVERRIDE_MEM(
        pay,
        (Payroll & payroll, boost::openmethod::virtual_ptr<const Employee>),
        double) {
        double amount = 5000.0;
        payroll.update_balance(-amount);
        return amount;
    }

    BOOST_OPENMETHOD_OVERRIDE_MEM(
        pay,
        (Payroll & payroll, boost::openmethod::virtual_ptr<const Salesman> emp),
        double) {
        using self = BOOST_OPENMETHOD_OVERRIDER_MEM(
            Payroll, pay,
            (Payroll&, boost::openmethod::virtual_ptr<const Salesman>), double);
        double base = self::method_type::next<self::fn>(payroll, emp);
        double commission = emp->sales * 0.05;
        payroll.update_balance(-commission);
        return base + commission;
    }
};
// end::payroll[]

// ...and let's not forget to register the classes
BOOST_OPENMETHOD_CLASSES(Employee, Salesman);

// tag::main[]
int main() {
    boost::openmethod::initialize();

    Dog snoopy;
    Cat felix;
    std::cout << "poke dog: " << Zoo::poke(snoopy) << "\n"; // bark
    std::cout << "poke cat: " << Zoo::poke(felix) << "\n";  // hiss

    Payroll payroll;
    Employee bill;
    Salesman bob;
    bob.sales = 100'000.0;

    std::cout << "pay bill: $" << pay(payroll, bill) << "\n";         // $5000
    std::cout << "pay bob: $" << pay(payroll, bob) << "\n";           // 10000
    std::cout << "remaining balance: $" << payroll.balance() << "\n"; // $985000
}
// end::main[]
