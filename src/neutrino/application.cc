#include <neutrino/application.hh>
#include <sdlpp/video/display.hh>
#include <sdlpp/input/gamepad.hh>
#include <failsafe/enforce.hh>
#include <failsafe/logger/backend/sdl_backend.hh>

#include <musac_backends/sdl3/sdl3_backend.hh>
#include <musac/audio_device.hh>
#include <musac/audio_source.hh>
#include <musac/audio_system.hh>
#include <musac/stream.hh>
#include <musac/error.hh>
#include <musac/codecs/register_codecs.hh>

#include <algorithm>
#include <array>
#include <vector>

namespace neutrino {
    application* g_app = nullptr;

    struct gamepad_state {
        sdlpp::gamepad pad;
        sdlpp::joystick_id instance_id = 0;
        std::array<sdlpp::button_state, static_cast<size_t>(sdlpp::gamepad_button::max)> buttons{};
        std::array<float, static_cast<size_t>(sdlpp::gamepad_axis::max)> axes{};

        explicit gamepad_state(sdlpp::gamepad&& p, sdlpp::joystick_id id)
            : pad(std::move(p)), instance_id(id) {}
    };

    struct application::impl {
        application_config m_cfg;
        sdlpp::renderer* m_renderer = nullptr;
        sdlpp::window* m_window = nullptr;

        std::shared_ptr <musac::audio_backend> m_audio_backend;
        std::shared_ptr<musac::decoders_registry> m_musac_codecs;

        std::vector<gamepad_state> m_gamepads;

        explicit impl(const application_config& cfg)
            : m_cfg(cfg) {
            m_audio_backend = musac::create_sdl3_backend();
            m_musac_codecs = musac::create_registry_with_all_codecs();

            if (!musac::audio_system::init(m_audio_backend, m_musac_codecs)) {
                THROW_RUNTIME("Failed to initialize audio system");
            }
        }
    };

    application::application()
        : m_pimpl(std::make_unique <impl>(application_config{})) {
        ENFORCE(!g_app)("neutrino::application was already initialized");
        g_app = this;
    }

    application::application(const application_config& cfg)
        : m_pimpl(std::make_unique <impl>(cfg)) {
        ENFORCE(!g_app)("neutrino::application was already initialized");
        g_app = this;
    }

    application::~application() {
        musac::audio_system::done();
        g_app = nullptr;
    }

    sdlpp::window_config application::get_window_config() {
        if (m_pimpl->m_cfg.width <= 0 || m_pimpl->m_cfg.height <= 0) {
            if (auto d = sdlpp::display_manager::get_primary_display()) {
                if (auto mode = d->get_desktop_mode()) {
                    if (m_pimpl->m_cfg.width <= 0) {
                        m_pimpl->m_cfg.width = static_cast <int>(mode->width);
                    }
                    if (m_pimpl->m_cfg.height <= 0) {
                        m_pimpl->m_cfg.height = static_cast <int>(mode->height);
                    }
                }
            }
            // Fallbacks if query fails
            if (m_pimpl->m_cfg.width <= 0) m_pimpl->m_cfg.width = 1280;
            if (m_pimpl->m_cfg.height <= 0) m_pimpl->m_cfg.height = 720;
        }

        return {
            m_pimpl->m_cfg.title,
            m_pimpl->m_cfg.width,
            m_pimpl->m_cfg.height,
            m_pimpl->m_cfg.flags,
            m_pimpl->m_cfg.target_fps
        };
    }

    void application::on_ready() {
        game_application::on_ready();
        failsafe::logger::set_backend(failsafe::logger::backends::make_sdl_backend());
        m_pimpl->m_renderer = &get_renderer();
        m_pimpl->m_window = &get_window();
        ready();
    }

    void application::on_update(float dt) {
        // Clear transient button states
        for (auto& pad : m_pimpl->m_gamepads) {
            for (auto& btn : pad.buttons) {
                btn.pressed = false;
                btn.released = false;
            }
        }
        update(dt);
    }

    void application::on_render(sdlpp::renderer& r) {
        render(r);
    }

    void application::handle_event(const sdlpp::event& e) {
        if (e.type() == sdlpp::event_type::gamepad_added) {
            if (auto* dev_ev = e.as<sdlpp::gamepad_device_event>()) {
                auto pad_res = sdlpp::gamepad::open(dev_ev->which);
                if (pad_res) {
                    m_pimpl->m_gamepads.emplace_back(std::move(*pad_res), dev_ev->which);
                }
            }
        } else if (e.type() == sdlpp::event_type::gamepad_removed) {
            if (auto* dev_ev = e.as<sdlpp::gamepad_device_event>()) {
                auto it = std::find_if(m_pimpl->m_gamepads.begin(), m_pimpl->m_gamepads.end(), [&](const auto& g) {
                    return g.instance_id == dev_ev->which;
                });
                if (it != m_pimpl->m_gamepads.end()) {
                    m_pimpl->m_gamepads.erase(it);
                }
            }
        } else if (e.type() == sdlpp::event_type::gamepad_button_down) {
            if (auto* btn_ev = e.as<sdlpp::gamepad_button_event>()) {
                auto it = std::find_if(m_pimpl->m_gamepads.begin(), m_pimpl->m_gamepads.end(), [&](const auto& g) {
                    return g.instance_id == btn_ev->which;
                });
                if (it != m_pimpl->m_gamepads.end() && btn_ev->button < static_cast<Uint8>(sdlpp::gamepad_button::max)) {
                    auto& btn = it->buttons[btn_ev->button];
                    btn.pressed = true;
                    btn.held = true;
                }
            }
        } else if (e.type() == sdlpp::event_type::gamepad_button_up) {
            if (auto* btn_ev = e.as<sdlpp::gamepad_button_event>()) {
                auto it = std::find_if(m_pimpl->m_gamepads.begin(), m_pimpl->m_gamepads.end(), [&](const auto& g) {
                    return g.instance_id == btn_ev->which;
                });
                if (it != m_pimpl->m_gamepads.end() && btn_ev->button < static_cast<Uint8>(sdlpp::gamepad_button::max)) {
                    auto& btn = it->buttons[btn_ev->button];
                    btn.released = true;
                    btn.held = false;
                }
            }
        } else if (e.type() == sdlpp::event_type::gamepad_axis_motion) {
            if (auto* axis_ev = e.as<sdlpp::gamepad_axis_event>()) {
                auto it = std::find_if(m_pimpl->m_gamepads.begin(), m_pimpl->m_gamepads.end(), [&](const auto& g) {
                    return g.instance_id == axis_ev->which;
                });
                if (it != m_pimpl->m_gamepads.end() && axis_ev->axis < static_cast<Uint8>(sdlpp::gamepad_axis::max)) {
                    float val = static_cast<float>(axis_ev->value) / 32767.0f;
                    if (val < -1.0f) val = -1.0f;
                    if (val > 1.0f) val = 1.0f;
                    it->axes[axis_ev->axis] = val;
                }
            }
        }

        event(e);
    }


    sdlpp::button_state application::get_gamepad_button_state(int gamepad_index, sdlpp::gamepad_button button) const noexcept {
        if (gamepad_index >= 0 && static_cast<size_t>(gamepad_index) < m_pimpl->m_gamepads.size()) {
            auto btn_idx = static_cast<size_t>(button);
            if (btn_idx < m_pimpl->m_gamepads[gamepad_index].buttons.size()) {
                return m_pimpl->m_gamepads[gamepad_index].buttons[btn_idx];
            }
        }
        return {};
    }

    float application::get_gamepad_axis(int gamepad_index, sdlpp::gamepad_axis axis) const noexcept {
        if (gamepad_index >= 0 && static_cast<size_t>(gamepad_index) < m_pimpl->m_gamepads.size()) {
            auto axis_idx = static_cast<size_t>(axis);
            if (axis_idx < m_pimpl->m_gamepads[gamepad_index].axes.size()) {
                return m_pimpl->m_gamepads[gamepad_index].axes[axis_idx];
            }
        }
        return 0.0f;
    }
}
