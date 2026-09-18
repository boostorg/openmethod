// Copyright (c) 2017-2026 Jean-Louis Leroy
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt
// or copy at http://www.boost.org/LICENSE_1_0.txt)

// tag::content[]
#include <boost/openmethod.hpp>
#include <boost/openmethod/initialize.hpp>
#include <iostream>

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
        balance_ += amount;
    }

    static auto pay_employee(
        Payroll& payroll, boost::openmethod::virtual_ptr<const Employee>)
        -> double {
        double pay = 5000.0;
        payroll.update_balance(-pay);
        return pay;
    }

    static auto pay_salesman(
        Payroll& payroll, boost::openmethod::virtual_ptr<const Salesman> emp)
        -> double {
        double base = pay_employee(payroll, emp);
        double commission = emp->sales * 0.05;
        payroll.update_balance(-commission);
        return base + commission;
    }

    BOOST_OPENMETHOD_OVERRIDE_FN(
        pay,
        (Payroll & payroll, boost::openmethod::virtual_ptr<const Employee>),
        double, &Payroll::pay_employee, &Payroll::pay_salesman);
};
// end::payroll[]

// ...and let's not forget to register the classes
BOOST_OPENMETHOD_CLASSES(Employee, Salesman);

// tag::main[]
int main() {
    boost::openmethod::initialize();

    Payroll payroll;
    Employee bill;
    Salesman bob;
    bob.sales = 100'000.0;

    std::cout << "pay bill: $" << pay(payroll, bill) << "\n";         // $5000
    std::cout << "pay bob: $" << pay(payroll, bob) << "\n";           // 10000
    std::cout << "remaining balance: $" << payroll.balance() << "\n"; // $985000
}
// end::main[]
