//
// Created by igor on 24/11/2026.
//

#pragma once

#include <neutrino/detail/generic_handle.hh>

namespace neutrino {
    namespace detail {
        struct action_id_tag;
    }
    using action_id = detail::generic_handle<detail::action_id_tag>;
}
