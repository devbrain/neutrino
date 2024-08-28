//
// Created by igor on 8/27/24.
//

#ifndef NEUTRINO_SCENE_UI_SCENE_HH
#define NEUTRINO_SCENE_UI_SCENE_HH

#include <neutrino/scene/scene.hh>
#include <neutrino/imgui/imgui.h>

namespace neutrino {
    class NEUTRINO_EXPORT ui_scene : public scene {
        friend class scene_manager;
        public:
            ui_scene();
            ~ui_scene() override = default;
        protected:
            virtual void draw_imgui(sdl::renderer& renderer) = 0;
            virtual void setup_imgui();
            flags get_flags() const override;
        private:
            void initialize() override;
            void handle_input(const sdl::events::event_t& ev) override;
            void update(std::chrono::milliseconds delta_time) override;
            void render(sdl::renderer& renderer) override;
    };
}

#endif
