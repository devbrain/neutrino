#include <doctest/doctest.h>
#include <neutrino/neutrino.hh>

TEST_CASE("Neutrino scaffolding test") {
    CHECK(neutrino::NeutrinoLib::get_version() != nullptr);
}
