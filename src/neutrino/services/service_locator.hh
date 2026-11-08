//
// Created by igor on 02/07/2026.
//

#pragma once

#include <sdlpp/video/window.hh>
#include <sdlpp/video/renderer.hh>

#include "input/gamepads.hh"
#include "video/sprite/texture_registry.hh"
#include "video/sprite/sprites_manager.hh"


namespace neutrino {
    class application;
    class scenes_manager;
    class sound_system;
    class resource_cache;
    class sprite_cache;
    class fonts_manager;

    class service_locator {
        friend class application;
        public:
            static service_locator& instance();

            [[nodiscard]] sdlpp::window& get_window() const {
                return *m_window;
            }

            [[nodiscard]] sdlpp::renderer& get_renderer() const {
                return *m_renderer;
            }

            [[nodiscard]] scenes_manager& get_scenes_manager() const {
                return *m_scenes_manager;
            }

            // The services below are returned as pointers, unlike the ones
            // above: they are absent before the application exists and after
            // it is destroyed, and callers (input queries, sound_effect
            // destructors) must degrade gracefully in both cases.
            [[nodiscard]] application* get_application() const {
                return m_application;
            }

            [[nodiscard]] gamepads* get_gamepads() const {
                return m_gamepads;
            }

            [[nodiscard]] sound_system* get_sound_system() const {
                return m_sound_system;
            }

            [[nodiscard]] texture_registry* get_texture_registry() const {
                return m_texture_registry;
            }

            [[nodiscard]] sprites_manager* get_sprites_manager() const {
                return m_sprites_manager;
            }

            // The application's shared tileset render-resource cache. Null
            // before the application is ready and after it is torn down.
            [[nodiscard]] resource_cache* get_resource_cache() const {
                return m_resource_cache;
            }

            // The application's shared sprite cache. Null before the application is
            // ready and after it is torn down.
            [[nodiscard]] sprite_cache* get_sprite_cache() const {
                return m_sprite_cache;
            }

            [[nodiscard]] int get_max_texture_size() const {
                return m_max_texture_size;
            }

            [[nodiscard]] fonts_manager* get_fonts_manager() const {
                return m_fonts_manager;
            }
        private:
            void set_window(sdlpp::window& w);
            void set_renderer(sdlpp::renderer& r);
            void set_scenes_manager(scenes_manager& s);
            void set_application(application& a);
            void set_gamepads(gamepads& g);
            void set_sound_system(sound_system& s);
            void set_texture_registry(texture_registry& s);
            void set_sprites_manager(sprites_manager& s);
            void set_resource_cache(resource_cache& c);
            void set_sprite_cache(sprite_cache& c);
            void set_fonts_manager(fonts_manager& f);
            void clear_application(application& a) noexcept;

        private:
            sdlpp::renderer* m_renderer = nullptr;
            sdlpp::window* m_window = nullptr;
            scenes_manager* m_scenes_manager = nullptr;
            application*   m_application = nullptr;
            gamepads*      m_gamepads = nullptr;
            sound_system*  m_sound_system = nullptr;
            texture_registry* m_texture_registry = nullptr;
            sprites_manager* m_sprites_manager = nullptr;
            resource_cache* m_resource_cache = nullptr;
            sprite_cache* m_sprite_cache = nullptr;
            fonts_manager* m_fonts_manager = nullptr;
            int m_max_texture_size = 0;
    };

}
