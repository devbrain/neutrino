//
// Created by igor on 08/06/2021.
//

#include <string>
#include <doctest/doctest.h>
#include <neutrino/engine/observer.hh>


struct simple_observer : public neutrino::engine::observer<int> {
    int ivalue = 0;

    void on_event(const int& x) override {
        ivalue = x;
    }

};

struct complext_observer : public neutrino::engine::observer<int, double> {
    int ivalue = 0;
    double dvalue = 0;

    void on_event(const int& x) override {
        ivalue = x;
    }

    void on_event(const double& x) override {
        dvalue = x;
    }
};

TEST_SUITE("observer/subject") {
    TEST_CASE("Test simple observer") {
        simple_observer so;
        neutrino::engine::publisher<int> subj;

        subj.attach(&so);
        subj.notify(1);

        REQUIRE(so.ivalue == 1);

        simple_observer so2;
        subj.attach(&so2);
        subj.notify(2);

        REQUIRE(so.ivalue == 2);
        REQUIRE(so2.ivalue == 2);
    }

    TEST_CASE("Test complex observer") {
        simple_observer so;
        complext_observer co;

        neutrino::engine::publisher<int, double> subj;

        subj.attach(&so);
        subj.attach(&co);

        subj.notify(1);

        REQUIRE(so.ivalue == 1);
        REQUIRE(co.ivalue == 1);
        REQUIRE(co.dvalue == 0);

        subj.notify(3.14);
        REQUIRE(so.ivalue == 1);
        REQUIRE(co.ivalue == 1);
        REQUIRE(co.dvalue == 3.14);

        simple_observer so2;
        subj.attach(&so2);
        subj.notify(2);

        REQUIRE(so.ivalue == 2);
        REQUIRE(so2.ivalue == 2);
        REQUIRE(co.ivalue == 2);
        REQUIRE(co.dvalue == 3.14);
    }

    int z = 0;
    void f(const int& x) {z = x;}

    TEST_CASE("Test functional observer") {
        int x = 0;

        neutrino::engine::publisher<int> subj;
        subj.attach([&x](const int & a){x = a;});
        subj.attach(f);

        subj.notify(5);

        REQUIRE(x == 5);
        REQUIRE(z == 5);

    }
}
