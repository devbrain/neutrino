#include <neutrino/application.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <sdlpp/video/display.hh>
#include <SDL3/SDL.h>
#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>
#include <failsafe/logger/backend/sdl_backend.hh>

#include "audio/sound_system.hh"
#include "input/gamepads.hh"
#include "scene/scenes_manager.hh"
#include "services/service_locator.hh"
#include "video/sprite/sprites_manager.hh"
#include "video/sprite/texture_registry.hh"
#include <neutrino/video/globals.hh>
#include <neutrino/video/world/resource_cache.hh>

namespace neutrino {
    namespace {
        [[nodiscard]] SDL_RendererLogicalPresentation to_sdl(scale_mode mode) noexcept {
            switch (mode) {
                case scale_mode::letterbox:      return SDL_LOGICAL_PRESENTATION_LETTERBOX;
                case scale_mode::integer_scale:  return SDL_LOGICAL_PRESENTATION_INTEGER_SCALE;
                case scale_mode::overscan:       return SDL_LOGICAL_PRESENTATION_OVERSCAN;
                case scale_mode::stretch:        return SDL_LOGICAL_PRESENTATION_STRETCH;
            }
            return SDL_LOGICAL_PRESENTATION_LETTERBOX;
        }
    }

    struct application::impl {
        application_config m_cfg;

        sound_system m_sound_system;
        gamepads m_gamepads;
        scenes_manager m_scenes_manager;
        texture_registry m_textures;
        sprites_manager m_sprites;
        // The shared tileset render-resource cache (Tier 2). Held by pointer and
        // torn down explicitly in ~application while m_textures/m_sprites are still
        // published, since destroy_bundle unregisters through the service_locator.
        std::unique_ptr<resource_cache> m_resource_cache;
        // Set once the first scene enters the stack; an empty stack after
        // that means the game is over, while a scene-less application keeps
        // running on the plain update()/render() callbacks.
        bool m_had_scenes = false;
        // A scene faulted in update_physics this frame: its pop is already
        // enqueued, so skip its render to avoid enqueueing a second pop.
        bool m_scene_faulted = false;
        // Last render_size() delivered via on_resize, so a window_pixel_size_changed
        // that leaves the render space unchanged (e.g. a logical-mode resize) does
        // not spuriously re-fire the hook.
        dim m_last_render_size{0, 0};

        explicit impl(const application_config& cfg)
            : m_cfg(cfg) {
        }
    };

    application::application()
        : application(application_config{}) {
    }

    application::application(const application_config& cfg)
        : m_pimpl(std::make_unique <impl>(cfg)) {
        auto& services = service_locator::instance();
        ENFORCE(!services.get_application())("neutrino::application was already initialized");

    }

    application::~application() {
        // Normally on_quit() already reset the cache while the render services were
        // live; this is an idempotent fallback for an abnormal teardown path.
        m_pimpl->m_resource_cache.reset();
        service_locator::instance().clear_application(*this);
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

        auto flags = m_pimpl->m_cfg.flags;
        if (m_pimpl->m_cfg.fullscreen) {
            flags = flags | sdlpp::window_flags::fullscreen;
        }
        if (m_pimpl->m_cfg.high_pixel_density) {
            flags = flags | sdlpp::window_flags::high_pixel_density;
        }

        return {
            m_pimpl->m_cfg.title,
            m_pimpl->m_cfg.width,
            m_pimpl->m_cfg.height,
            flags,
            m_pimpl->m_cfg.target_fps
        };
    }

    void application::on_ready() {
        game_application::on_ready();
        failsafe::logger::set_backend(failsafe::logger::backends::make_sdl_backend());

        service_locator::instance().set_application(*this);
        service_locator::instance().set_renderer(get_renderer());
        // The game_application path leaves vsync off; honor the config so apps present
        // without tearing by default (fast full-screen scrolling otherwise tears).
        if (auto vs = get_renderer().set_vsync(m_pimpl->m_cfg.vsync); !vs) {
            LOG_ERROR("Failed to set vsync:", vs.error());
        }
        // Logical presentation (if a design resolution was configured) must be set
        // before scenes enter, so their first on_resize sees the render coordinate
        // space, not the raw drawable size.
        if (m_pimpl->m_cfg.logical_size) {
            const auto ls = *m_pimpl->m_cfg.logical_size;
            if (!SDL_SetRenderLogicalPresentation(
                    get_renderer().get(), ls.width, ls.height, to_sdl(m_pimpl->m_cfg.scale))) {
                LOG_ERROR("Failed to set logical presentation:", SDL_GetError());
            }
        }
        m_pimpl->m_last_render_size = render_size();
        service_locator::instance().set_window(get_window());
        service_locator::instance().set_gamepads(m_pimpl->m_gamepads);
        service_locator::instance().set_sound_system(m_pimpl->m_sound_system);
        service_locator::instance().set_scenes_manager(m_pimpl->m_scenes_manager);
        service_locator::instance().set_texture_registry(m_pimpl->m_textures);
        service_locator::instance().set_sprites_manager(m_pimpl->m_sprites);
        m_pimpl->m_resource_cache = std::make_unique<resource_cache>();
        service_locator::instance().set_resource_cache(*m_pimpl->m_resource_cache);

        // sdlpp initializes SDL with video|events only; without the gamepad
        // subsystem SDL emits no gamepad events at all. Ref-counted, and
        // SDL_Quit at teardown releases it.
        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
            LOG_ERROR("Failed to initialize gamepad subsystem:", SDL_GetError());
        }
        ready();
        if (auto scene = create_initial_scene()) {
            m_pimpl->m_scenes_manager.push_scene_sync(std::move(scene));
            // Even if the scene's on_enter threw and was rolled back, the
            // game committed to scene mode: an empty stack now means quit
            // instead of idling on a black screen forever.
            m_pimpl->m_had_scenes = true;
        }
    }

    void application::on_update(float dt) {
        m_pimpl->m_sprites.update(sprite_animation_duration{dt * 1000.0f});

        if (!m_pimpl->m_scenes_manager.empty()) {
            m_pimpl->m_had_scenes = true;
            // Safety net: an exception escaping to SDL would become
            // SDL_APP_FAILURE and terminate the process. Log it and drop the
            // top scene (it's clearly broken) so the next frame runs a
            // known-good one. The pop is enqueued via the SDL event queue and
            // takes effect when the next handle_event drains it.
            try {
                m_pimpl->m_scenes_manager.update_physics(frame_duration{dt * 1000.0f});
            } catch (const std::exception& ex) {
                LOG_ERROR("Scene update threw:", ex.what());
                pop_scene();
                m_pimpl->m_scene_faulted = true;
            } catch (...) {
                LOG_ERROR("Scene update threw an unknown exception");
                pop_scene();
                m_pimpl->m_scene_faulted = true;
            }
        } else if (m_pimpl->m_had_scenes) {
            quit();
            return;
        }

        update(dt);
    }

    void application::on_render(sdlpp::renderer& r) {
        r.set_draw_color(sdlpp::colors::black);
        r.clear();
        if (m_pimpl->m_scene_faulted) {
            m_pimpl->m_scene_faulted = false;
        } else {
            try {
                m_pimpl->m_scenes_manager.render(frame_duration{delta_time() * 1000.0f});
            } catch (const std::exception& ex) {
                LOG_ERROR("Scene render threw:", ex.what());
                pop_scene();
            } catch (...) {
                LOG_ERROR("Scene render threw an unknown exception");
                pop_scene();
            }
        }
        r.present();

        // End of frame: clear transient gamepad state (pressed/released) at
        // the same point where sdlpp clears keyboard/mouse transients.
        // Events for the next frame arrive after this, so a press stays
        // visible to scene updates for exactly one full frame. Clearing at
        // the start of on_update would wipe presses delivered just before
        // the iterate and they would never be observable.
        m_pimpl->m_gamepads.clear_state();
    }

    void application::handle_event(const sdlpp::event& e) {
        if (auto change = m_pimpl->m_gamepads.handle_event(e)) {
            if (change->connected) {
                on_gamepad_connected(change->index);
            } else {
                on_gamepad_disconnected(change->index);
            }
        }
        // The drawable changed (resize, fullscreen toggle, HiDPI move). Recompute
        // the render space and, if it actually changed, notify the active scene.
        // In logical mode the render space is fixed, so this dedups to nothing.
        if (e.type() == sdlpp::event_type::window_pixel_size_changed) {
            const dim now = render_size();
            if (now.width != m_pimpl->m_last_render_size.width
                || now.height != m_pimpl->m_last_render_size.height) {
                m_pimpl->m_last_render_size = now;
                m_pimpl->m_scenes_manager.notify_resize(now);
            }
        }
        try {
            m_pimpl->m_scenes_manager.handle_action(e);
        } catch (const std::exception& ex) {
            LOG_ERROR("Scene event handler threw:", ex.what());
        } catch (...) {
            LOG_ERROR("Scene event handler threw an unknown exception");
        }
        event(e);
    }

    void application::on_quit() noexcept {
        // Tear down the scene stack while the renderer, window and audio are
        // still alive — scenes may release textures / sounds in on_exit().
        try {
            m_pimpl->m_scenes_manager.finish();
        } catch (...) {
            LOG_ERROR("Scene teardown threw during shutdown");
        }
        // Now that scenes (and their world_renderers) have released their handles,
        // tear the cache down while the renderer, texture registry and sprite
        // manager are still alive — destroy_bundle destroys GPU textures and
        // unregisters through the still-published services.
        m_pimpl->m_resource_cache.reset();
        game_application::on_quit();
        service_locator::instance().clear_application(*this);
    }

}
