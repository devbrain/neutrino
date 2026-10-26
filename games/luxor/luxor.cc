//
// Created by igor on 11/10/2026.
//

#include <iostream>
#include <filesystem>

#include <resources/storage/game.hh>
#include <resources/storage/vfs.hh>
#include <luxor/resources/loader.hh>

#include <luxor/scenes/play_scene.hh>
#include <neutrino/application.hh>
#include <neutrino/entry_point.hh>
// =========================================================================
// The Application: tutorial_app
// =========================================================================
// Logical design resolution for our platformer.
// Operating in 800x600 coordinates decoupled from physical monitor resolution.
constexpr int logical_width = 800;
constexpr int logical_height = 600;

// Window dimensions at startup.
constexpr int window_width = 800;
constexpr int window_height = 600;
namespace {
    class luxor_app final : public neutrino::application {
        public:
            luxor_app()
                : neutrino::application(make_config()) {
            }

        protected:
            std::unique_ptr <neutrino::base_scene> create_initial_scene() override {
                return std::make_unique <luxor::play_scene>();
            }

        private:
            static neutrino::application_config make_config() {
                neutrino::application_config cfg;
                cfg.title = "Luxor";
                cfg.width = window_width;
                cfg.height = window_height;
                // cfg.flags = sdlpp::window_flags::maximized;

                cfg.logical_size = neutrino::dim{logical_width, logical_height};
                cfg.scale = neutrino::scale_mode::letterbox;

                return cfg;
            }
    };
}

#if 0
int main(int argc, char* argv[]) {
    const std::filesystem::path path = "/home/igor/games/luxor/Luxor Bundle Pack/Luxor";

    auto game_kind = luxor::rc::game::probe(path);
    if (game_kind != luxor::rc::game_kind::luxor_1) {
        return 1;
    }

    luxor::rc::vfs vfs;
    vfs.mount("", path / "data/data.mjz");
    vfs.mount("", path / "data/english.mjz");
    vfs.set_locale("english");

    luxor::rc::load_sprite(vfs, "data/sprites/game/ball_1.spr");

    // vfs.dump("/home/igor/games/luxor_dump");

    return 0;
}
#endif

SDLPP_MAIN(luxor_app)