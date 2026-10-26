//
// Created by igor on 23/10/2026.
//

#include <random>
#include <vector>

#include <neutrino/simplex/entry_point.hh>
#include <neutrino/simplex/application.hh>
#include <euler/vector/vector.hh>

#include "neutrino/video/draw.hh"

using namespace neutrino;

namespace {
    class random {
        public:
            random()
                : mt{std::random_device{}()}, dist(0, 1.0) {
            }

            float rand() const noexcept {
                return dist(mt);
            }

        private:
            mutable std::mt19937 mt;
            mutable std::uniform_real_distribution <float> dist;
    };

    struct star {
        euler::vec3 <float> pos;
        float speed_scale = 1.0f;
    };

    class starfield : public simplex::application {
        public:
            starfield(int width_px, int height_px, const std::string& title, float scale)
                : application(width_px, height_px, title, scale),
                  m_stars(250) {
            }

            bool on_create() override {
                for (auto& s : m_stars) {
                    reinint_star(s, true);
                }
                return true;
            }

            bool on_update(float dt, const input_snapshot& input) override {
                if (input.key(sdlpp::scancode::escape).pressed) {
                    return false;
                }

                const auto [w, h] = render_size();
                const auto center_x = static_cast <float>(w) / 2.0f;
                const auto center_y = static_cast <float>(w) / 2.0f;
                constexpr float focal_length = 400.0f;
                // update position
                for (auto& s : m_stars) {
                    s.pos.z() -= 20.0f*s.speed_scale*dt;
                }
                // draw
                for (auto& s : m_stars) {
                    bool invalid = false;
                    if (std::abs(s.pos.z()) < 1) {
                        reinint_star(s, true);
                    } else {
                        // Perspective divide: screen = center + (world * focal) / z
                        const float inv_z = focal_length / s.pos.z();
                        const float x = 0 + s.pos.x() * inv_z;
                        const float y = 0 + s.pos.y() * inv_z;
                        if (x <= 0 || x >= w || y <=0 || y >= h) {
                            reinint_star(s, true);
                        } else {
                            set_draw_color(sdlpp::colors::red);
                            draw_point(render_pos{x, y});
                        }
                    }
                }
                return true;
            }

            void on_destroy() override {
            }

        private:
            void reinint_star(star& s, bool random_z) const {
                const auto [w, h] = render_size();
                const float rx = m_rng.rand() * static_cast <float>(w);
                const float ry = m_rng.rand() * static_cast <float>(h);
                const float rz = random_z ? m_rng.rand() * 900.0f + 100.0f : 1000.0f;

                s.pos = {rx, ry, rz};
                s.speed_scale = 0.8f + m_rng.rand() * 0.4f;
            }

        private:
            random m_rng;
            std::vector <star> m_stars;
    };
}

SIMPLEX_MAIN(starfield, 640, 480, "Stars...", 1.0);
