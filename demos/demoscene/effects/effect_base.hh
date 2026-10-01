#pragma once

#include <string_view>
#include <neutrino/input/input_snapshot.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/geometry_types.hh>

namespace demoscene {

/**
 * @brief Base interface for educational demoscene effects.
 */
class effect_base {
public:
    virtual ~effect_base() = default;

    virtual void on_enter() {}
    virtual void on_exit() {}

    /**
     * @brief Update simulation and handle effect-specific input.
     */
    virtual void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) = 0;

    /**
     * @brief Render the effect into the designated viewport (e.g. 640x400 or scaled).
     */
    virtual void render(const neutrino::rect& viewport) = 0;

    // ------------------------------------------------------------------------
    // Educational Metadata
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    [[nodiscard]] virtual std::string_view original_file() const noexcept = 0;
    [[nodiscard]] virtual std::string_view author() const noexcept = 0;
    [[nodiscard]] virtual std::string_view math_formula() const noexcept = 0;
    [[nodiscard]] virtual std::string_view description() const noexcept = 0;
    [[nodiscard]] virtual std::string_view controls_hint() const noexcept {
        return "None";
    }
};

} // namespace demoscene
