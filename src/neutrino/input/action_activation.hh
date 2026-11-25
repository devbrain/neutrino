//
// Created by igor on 24/11/2026.
//

#pragma once

#include <variant>

#include <neutrino/input/hotkey.hh>
#include <neutrino/input/mouse_click.hh>
#include <neutrino/input/gamepad_button.hh>


namespace neutrino::detail {
    using action_activation = std::variant<std::monostate, hotkey, mouse_click, gamepad_button>;
}