//
// Created by igor on 12/10/2026.
//

#pragma once

#include <failsafe/expected.hh>

namespace luxor::rc {
    template <typename T, typename E = failsafe::error_type>
    using expected = failsafe::expected<T, E>;

    template <typename E>
    using unexpected = failsafe::unexpected<E>;

    using failsafe::unexpect_t;
    using failsafe::unexpect;
    using failsafe::bad_expected_access;

    using failsafe::make_unexpected;
    using failsafe::make_unexpectedf;

    using failsafe::using_std_expected;
    using failsafe::expected_implementation;

    using error_type = failsafe::error_type;
    using result = failsafe::result;
}
