//
// Created by igor on 25/11/2026.
//

#include <doctest/doctest.h>

#include "events/event_types.hh"

#include <failsafe/logger/backend/capture_backend.hh>

#include <cstdint>

namespace {
    enum class registered_events : std::int32_t { ping };
    enum class never_registered_events : std::int32_t { ping };

    using registered = neutrino::custom_event <registered_events>;
    using never_registered = neutrino::custom_event <never_registered_events>;
}

TEST_SUITE("neutrino::events") {
    TEST_CASE("registering again keeps the type and logs nothing: every application after the first does") {
        registered::init();
        const auto type = registered::category();
        CHECK(type != neutrino::detail::INVALID_EVENT);

        failsafe::logger::backends::scoped_log_capture logs;
        registered::init();
        CHECK(registered::category() == type);
        CHECK(logs.count("registered") == 0);
    }

    TEST_CASE("draining events of a type never registered does nothing: startup failed before on_ready") {
        CHECK_NOTHROW(never_registered::drain());
        CHECK(never_registered::category() == neutrino::detail::INVALID_EVENT);
    }
}
