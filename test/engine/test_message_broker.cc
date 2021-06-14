//
// Created by igor on 09/06/2021.
//

#include <doctest/doctest.h>
#include <neutrino/engine/message_broker.hh>

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

TEST_SUITE("message broker") {
    TEST_CASE("complex observer") {
        complext_observer obs;
        neutrino::engine::message_broker mb;

        mb.attach(&obs);

        REQUIRE(obs.ivalue == 0);
    }
}