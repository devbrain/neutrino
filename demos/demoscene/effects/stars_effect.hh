#pragma once

#include "effect_base.hh"

#include <vector>
#include <euler/vector/vector.hh>

namespace demoscene {

/**
 * @brief Reconstruction of Bas van Gaalen's Warp Starfield (STARS.PAS).
 *
 * Demonstrates:
 * - 3D depth perspective projection using euler::vec3<float>.
 * - Anti-aliased velocity motion streaks with neutrino::draw_line_aa().
 * - Dynamic warp acceleration (hyperdrive mode).
 */
class stars_effect final : public effect_base {
public:
    stars_effect();

    void on_enter() override;
    void update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override;
    void render(const neutrino::rect& viewport) override;

    [[nodiscard]] std::string_view name() const noexcept override {
        return "3D Warp Starfield";
    }
    [[nodiscard]] std::string_view original_file() const noexcept override {
        return "STARS.PAS (1994)";
    }
    [[nodiscard]] std::string_view author() const noexcept override {
        return "Bas van Gaalen";
    }
    [[nodiscard]] std::string_view math_formula() const noexcept override {
        return "x' = xc + x·D / z; y' = yc + y·D / z; z ← z - v·dt";
    }
    [[nodiscard]] std::string_view description() const noexcept override {
        return "Perspective-correct starfield with anti-aliased velocity streaks and warp acceleration.";
    }
    [[nodiscard]] std::string_view controls_hint() const noexcept override {
        return "Up/Down: Warp Speed | Space: Hyperdrive Burst | R: Reset";
    }

private:
    struct star {
        euler::vec3<float> pos;
        float prev_proj_x = 0.0f;
        float prev_proj_y = 0.0f;
        float speed_scale = 1.0f;
    };

    void respawn_star(star& s, bool random_z = false);

    static constexpr size_t star_count = 250;
    std::vector<star> m_stars;
    float m_warp_speed = 350.0f;
    bool m_hyperdrive = false;
};

} // namespace demoscene
