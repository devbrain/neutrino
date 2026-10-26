//
// Created by igor on 18/10/2026.
//

#include <cmath>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <sdlpp/core/log.hh>
#include <neutrino/simplex/application.hh>

#include "simplex/detail/application_adapter.hh"

namespace neutrino::simplex::detail {
    namespace {
        application_config make_config(int width_px, int height_px, const std::string& title, float scale) {
            if (width_px <= 0 || height_px <= 0 || !std::isfinite(scale) || scale <= 0.0f) {
                throw std::invalid_argument("Simplex dimensions and scale must be positive; scale must be finite");
            }
            // Do not round the input dimensions through float: INT_MAX rounds
            // up to an unrepresentable int even at scale 1. Check before casting.
            const double width = static_cast<double>(width_px) * static_cast<double>(scale);
            const double height = static_cast<double>(height_px) * static_cast<double>(scale);
            constexpr auto maximum = static_cast<double>(std::numeric_limits<int>::max());
            if (width < 1.0 || height < 1.0 || width > maximum || height > maximum) {
                throw std::invalid_argument("Simplex scaled window dimensions must fit positive int values");
            }

            application_config cfg;
            cfg.title = title;
            cfg.width = static_cast<int>(width);
            cfg.height = static_cast<int>(height);
            cfg.logical_size = neutrino::dim{width_px, height_px};
            cfg.scale = scale_mode::integer_scale;
            return cfg;
        }

        template <typename T>
        T checked(sdlpp::expected<T, std::string> result) {
            if (!result) throw std::runtime_error(result.error());
            if constexpr (!std::is_void_v<T>) return std::move(*result);
        }

        void log_cleanup_error(const char* message) noexcept {
            try {
                SDLPP_LOG_APP_ERROR("Simplex on_destroy failed: ", message);
            } catch (...) {
                // Cleanup must complete even if formatting the diagnostic fails.
            }
        }

        // Keep callback drawing in the persistent canvas, restoring the previous
        // target even when a callback throws or uses nested render targets.
        class canvas_scope {
            public:
                canvas_scope(sdlpp::renderer& renderer, const sdlpp::texture& canvas, sdlpp::fcolor& color)
                    : m_renderer(renderer), m_target(renderer), m_color(color) {
                    checked(renderer.set_target(canvas));
                    checked(renderer.set_draw_color_float(color));
                }

                void restore() {
                    m_color = checked(m_renderer.get_draw_color_float());
                    checked(m_target.restore());
                }

                canvas_scope(const canvas_scope&) = delete;
                canvas_scope& operator=(const canvas_scope&) = delete;

            private:
                sdlpp::renderer& m_renderer;
                sdlpp::renderer::target_guard m_target;
                sdlpp::fcolor& m_color;
        };
    }

    application_adapter::application_adapter(simplex::application* owner, int width_px, int height_px,
                                             const std::string& title, float scale)
        : application(make_config(width_px, height_px, title, scale)),
          m_owner(owner), m_canvas_size{width_px, height_px} {
    }

    void application_adapter::ready() {
        auto& renderer = get_renderer();
        m_canvas = checked(sdlpp::texture::create(renderer, sdlpp::pixel_format_enum::RGBA8888,
                                                 sdlpp::texture_access::target,
                                                 m_canvas_size.width, m_canvas_size.height));
        checked(m_canvas.set_blend_mode(sdlpp::blend_mode::none));
        checked(m_canvas.set_scale_mode(sdlpp::scale_mode::nearest));

        try {
            canvas_scope canvas(renderer, m_canvas, m_canvas_color);
            // Logical presentation belongs to each target separately. Keep
            // render_size() meaningful while callbacks draw into the canvas.
            checked(renderer.set_logical_presentation(m_canvas_size.width, m_canvas_size.height,
                                                       sdlpp::logical_presentation::integer_scale));
            checked(renderer.clear());
            m_state = lifecycle::creating;
            if (!m_owner->on_create()) {
                throw std::runtime_error("Simplex on_create returned false");
            }
            canvas.restore();
            m_state = lifecycle::running;
        } catch (...) {
            stop();
            throw; // The entry point reports initialization failure, then calls teardown.
        }
    }

    void application_adapter::update_scene(float dt, const input_snapshot& input) {
        if (m_state != lifecycle::running || !is_running()) return;
        try {
            canvas_scope canvas(get_renderer(), m_canvas, m_canvas_color);
            get_renderer().set_draw_color(sdlpp::colors::black);
            get_renderer().clear();

            const bool keep_running = m_owner->on_update(dt, input);
            canvas.restore();
            if (!keep_running) stop();
        } catch (...) {
            stop();
            throw; // The entry point reports callback failure.
        }
    }

    void application_adapter::render_scene() {
        if (m_state != lifecycle::running || !is_running()) return;
        auto& renderer = get_renderer();
        const sdlpp::rect<float> destination{0.0f, 0.0f, static_cast<float>(m_canvas_size.width),
                                              static_cast<float>(m_canvas_size.height)};

        if (const auto result = renderer.copy(m_canvas, std::nullopt, destination); !result) {
            stop();
            throw std::runtime_error(result.error());
        }
    }

    void application_adapter::stop() noexcept {
        if (m_state == lifecycle::creating || m_state == lifecycle::running) {
            m_state = lifecycle::stopped;
        }
        quit();
    }

    void application_adapter::teardown() {
        if (m_state == lifecycle::destroyed) return;
        const bool entered_create = m_state != lifecycle::idle;
        // Stop and mark cleanup before invoking user code. Even a throwing or
        // reentrant cleanup must not allow further callbacks or invoke this hook twice.
        m_state = lifecycle::destroyed;
        quit();
        if (entered_create) {
            try {
                m_owner->on_destroy();
            } catch (const std::exception& error) {
                log_cleanup_error(error.what());
            } catch (...) {
                log_cleanup_error("unknown exception");
            }
        }
        m_canvas = sdlpp::texture{};
    }

}
