//
// Created by igor on 12/07/2026.
//

#include <fstream>

#include <neutrino/application.hh>
#include <sdlpp/app/entry_point.hh>

#include <ke/assets/registry.hh>
#include <ke/format/archive.hh>
#include <ke/assets/sprites.hh>
#include <ke/scenes/play_game_scene.hh>
#include <ke/resources/resources.hh>

namespace {
    // The sprite-gallery debug scene needs room for names, so the app runs larger than the
    // game's 320x200 (switch these back with play_game_scene).
    constexpr int window_width = 320;
    constexpr int window_height = 200;
}

class ke : public neutrino::application {
    public:
        ke()
            : application(make_config()) {
        }

    protected:
        // Default to the sprite gallery (built from m_res, loaded in ready()). Swap for
        // play_game_scene to run the game.
        std::unique_ptr <neutrino::base_scene> create_initial_scene() override {
            return std::make_unique <play_game_scene>();
        }

        void on_config(int argc, char* argv[]) override {
            if (argc == 2) {
                m_path_to_rs = argv[1];
            }
        }

        void ready() override {
            constexpr auto* ke_rsc_path = "/home/igor/games/ke/Krypton-Egg_DOS_EN/ke.rsc";
            std::ifstream ifs(m_path_to_rs.empty() ? ke_rsc_path : m_path_to_rs.c_str(), std::ios::binary);
            if (!ifs) {
                LOG_ERROR("ke: cannot open", ke_rsc_path);
                quit();
                return;
            }
            auto res = rs::parse(ifs);
            if (!res) {
                LOG_ERROR("ke: failed to parse resources");
                quit();
                return;
            }
            m_res = std::move(*res);
            if (m_res.levels.empty()) {
                LOG_ERROR("ke: archive has no levels");
                quit();
                return;
            }
            m_assets.levels = std::move(m_res.levels);
            rs::set_ke_assets(m_assets);
            rs::define_sprites(m_res);
            m_assets.m_resources = &m_res;
        }
    private:
        static neutrino::application_config make_config() {
            neutrino::application_config cfg;
            cfg.title = "Krypton Egg";
            cfg.width = window_width;
            cfg.height = window_height;
            cfg.flags = sdlpp::window_flags::resizable;
            // Fixed design resolution, integer-scaled to the window so a larger window shows
            // the same content crisply, not more.
            cfg.logical_size = neutrino::dim{window_width, window_height};
            cfg.scale = neutrino::scale_mode::integer_scale;
            return cfg;
        }
    private:
        std::string m_path_to_rs;
        rs::game_resources m_res;
        rs::ke_assets m_assets; // owned; published via set_ke_assets
};

SDLPP_MAIN(ke)
