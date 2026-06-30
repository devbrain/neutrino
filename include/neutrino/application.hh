//
// Created by igor on 30/06/2026.
//

#pragma once

#include <memory>
#include <string>

#include <neutrino/neutrino_export.h>
#include <sdlpp/app/game_application.hh>

namespace neutrino {
    struct application_config {
        std::string title = "Neutrino Application";
        int width = 1280;
        int height = 720;
        sdlpp::window_flags flags = sdlpp::window_flags::resizable;
        int target_fps = 60;
    };

    class NEUTRINO_EXPORT application : public sdlpp::game_application {
        public:
            application();
            explicit application(const application_config& cfg);
            ~application() override;

        protected:
            sdlpp::window_config get_window_config() override;
        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
    };
}
