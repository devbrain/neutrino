//
// Neutrino Drawing Tutorial 01: Primitives & UI
//
// Demonstrates:
// 1. Immediate-mode shape drawing with `<neutrino/video/draw.hh>`.
// 2. Health and stamina status gauges with borders and fills.
// 3. Low-health pulsing alerts.
// 4. Specialized line styles: regular, AA, thick, dashed, and dotted.
// 5. Physics diagnostics: bounding boxes, center crosses, and velocity arrows.
// 6. Proximity sensor circles with dynamic triggering.
//

#include <neutrino/application.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>

#include <sdlpp/app/entry_point.hh>
#include <sdlpp/video/color.hh>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

namespace {

    constexpr int logical_width = 640;
    constexpr int logical_height = 360;
    constexpr int window_width = 1280;
    constexpr int window_height = 720;

    void draw_gauge(int x, int y, int width, int height,
                    float current, float maximum,
                    const sdlpp::color& fill_color) {
        const float fraction = std::clamp(maximum > 0.0f ? current / maximum : 0.0f, 0.0f, 1.0f);
        const int fill_width = static_cast<int>(fraction * (width - 4));

        // 1. Dark background tray
        neutrino::draw_rect_fill(neutrino::rect{x, y, width, height}, sdlpp::color{25, 25, 25, 220});

        // 2. Value fill with inner padding
        if (fill_width > 0) {
            neutrino::draw_rect_fill(
                neutrino::rect{x + 2, y + 2, fill_width, height - 4},
                fill_color
            );
        }

        // 3. Outer border
        neutrino::draw_rect(neutrino::rect{x, y, width, height}, sdlpp::color{200, 200, 200, 255});
    }

    void draw_hud(float health, float max_health, float stamina, float max_stamina, float total_time_sec) {
        sdlpp::color health_color = sdlpp::color{46, 204, 113, 255};

        if (health / max_health < 0.25f) {
            const float pulse = (std::sin(total_time_sec * 8.0f) + 1.0f) * 0.5f;
            health_color = sdlpp::color{
                static_cast<uint8_t>(200 + pulse * 55),
                static_cast<uint8_t>(40 * (1.0f - pulse)),
                static_cast<uint8_t>(40 * (1.0f - pulse)),
                255
            };
        }

        draw_gauge(16, 16, 180, 18, health, max_health, health_color);
        draw_gauge(16, 38, 140, 12, stamina, max_stamina, sdlpp::color{52, 152, 219, 255});
    }

    void draw_detection_radius(neutrino::point center, int radius, bool player_detected) {
        const sdlpp::color fill_color = player_detected
            ? sdlpp::color{231, 76, 60, 60}
            : sdlpp::color{52, 152, 219, 40};

        const sdlpp::color border_color = player_detected
            ? sdlpp::color{231, 76, 60, 200}
            : sdlpp::color{52, 152, 219, 180};

        neutrino::draw_circle_fill(neutrino::circle{center, radius}, fill_color);
        neutrino::draw_circle(neutrino::circle{center, radius}, border_color);
    }

    void draw_velocity_vector(neutrino::point actor_pos, float vx, float vy) {
        const neutrino::point arrow_end{
            actor_pos.x + static_cast<int>(vx * 0.25f),
            actor_pos.y + static_cast<int>(vy * 0.25f)
        };

        neutrino::draw_arrow(
            actor_pos,
            arrow_end,
            /* head_size = */ 8,
            /* head_angle = */ 30.0f,
            /* thickness = */ 1.5f,
            sdlpp::colors::orange
        );
    }

    class primitives_demo_scene final : public neutrino::base_scene {
    public:
        void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override {
            if (in.pressed(sdlpp::scancode::escape)) {
                neutrino::pop_scene();
                return;
            }

            m_elapsed_sec += dt.count();

            // Simulate fluctuating health & stamina
            m_health = 50.0f + 45.0f * std::sin(m_elapsed_sec * 0.8f);
            m_stamina = 75.0f + 25.0f * std::cos(m_elapsed_sec * 1.5f);

            // Orbit moving actor around sensor center
            m_actor_pos.x = 420 + static_cast<int>(80.0f * std::cos(m_elapsed_sec * 2.0f));
            m_actor_pos.y = 180 + static_cast<int>(50.0f * std::sin(m_elapsed_sec * 2.0f));

            m_actor_vel_x = -160.0f * std::sin(m_elapsed_sec * 2.0f);
            m_actor_vel_y =  100.0f * std::cos(m_elapsed_sec * 2.0f);
        }

        void handle_action(const sdlpp::event&) override {}

        void render() override {
            // Dark navy background
            neutrino::draw_rect_fill(neutrino::rect{0, 0, logical_width, logical_height}, sdlpp::color{20, 24, 34, 255});

            // 1. Line styles showcase panel on the left
            render_line_showcase();

            // 2. Interactive proximity sensor on the right
            const neutrino::point sensor_center{420, 180};
            constexpr int sensor_radius = 90;
            const bool in_range = std::hypot(m_actor_pos.x - sensor_center.x, m_actor_pos.y - sensor_center.y) < sensor_radius;

            draw_detection_radius(sensor_center, sensor_radius, in_range);

            // 3. Trajectory line (dashed)
            neutrino::draw_line_dashed(sensor_center, m_actor_pos, 8, 4, sdlpp::colors::dark_gray);

            // 4. Actor bounding box and center cross
            neutrino::draw_rect(neutrino::rect{m_actor_pos.x - 16, m_actor_pos.y - 16, 32, 32}, sdlpp::colors::yellow);
            neutrino::draw_cross(m_actor_pos, 5, 1.0f, sdlpp::colors::white);

            // 5. Actor velocity vector arrow
            draw_velocity_vector(m_actor_pos, m_actor_vel_x, m_actor_vel_y);

            // 6. Player status HUD at top-left
            draw_hud(m_health, 100.0f, m_stamina, 100.0f, m_elapsed_sec);
        }

        [[nodiscard]] bool is_opaque() const override {
            return true;
        }

    private:
        void render_line_showcase() {
            // Panel background
            neutrino::draw_rect_fill(neutrino::rect{16, 68, 240, 276}, sdlpp::color{28, 34, 48, 200});
            neutrino::draw_rect(neutrino::rect{16, 68, 240, 276}, sdlpp::color{60, 70, 95, 255});

            // Standard line
            neutrino::draw_line(neutrino::point{30, 100}, neutrino::point{230, 100}, sdlpp::colors::gray);

            // Anti-aliased diagonal line
            neutrino::draw_line_aa(neutrino::point{30, 130}, neutrino::point{230, 160}, sdlpp::colors::light_blue);

            // Thick line (4px stroke)
            neutrino::draw_line_thick(neutrino::point{30, 190}, neutrino::point{230, 190}, 4.0f, sdlpp::colors::coral);

            // Dashed line (10px mark, 5px gap)
            neutrino::draw_line_dashed(neutrino::point{30, 230}, neutrino::point{230, 230}, 10, 5, sdlpp::colors::yellow);

            // Dotted line (dots spaced every 8px)
            neutrino::draw_line_dotted(neutrino::point{30, 270}, neutrino::point{230, 270}, 8, sdlpp::colors::white);

            // Arrow primitive
            neutrino::draw_arrow(neutrino::point{30, 310}, neutrino::point{130, 310}, 8, 30.0f, 1.5f, sdlpp::colors::lime_green);
            neutrino::draw_cross(neutrino::point{180, 310}, 6, 1.5f, sdlpp::colors::cyan);
        }

        float m_elapsed_sec = 0.0f;
        float m_health = 100.0f;
        float m_stamina = 100.0f;
        neutrino::point m_actor_pos{420, 180};
        float m_actor_vel_x = 0.0f;
        float m_actor_vel_y = 0.0f;
    };

    class tutorial_app final : public neutrino::application {
    public:
        tutorial_app()
            : neutrino::application(make_config()) {
        }

    protected:
        std::unique_ptr<neutrino::base_scene> create_initial_scene() override {
            return std::make_unique<primitives_demo_scene>();
        }

    private:
        static neutrino::application_config make_config() {
            neutrino::application_config cfg;
            cfg.title = "Neutrino Drawing Tutorial 01 - Primitives & UI";
            cfg.width = window_width;
            cfg.height = window_height;
            cfg.flags = sdlpp::window_flags::resizable;
            cfg.logical_size = neutrino::dim{logical_width, logical_height};
            cfg.scale = neutrino::scale_mode::letterbox;
            return cfg;
        }
    };

} // namespace

SDLPP_MAIN(tutorial_app)
