//
// Created by igor on 12/07/2026.
//

#include <fstream>
#include <array>       // TEMP: bonus-map dump
#include <cstdio>      // TEMP: bonus-map dump
#include <cstdlib>     // TEMP: bonus-map dump (getenv)
#include <string>      // TEMP: bonus-map dump
#include <vector>      // TEMP: bonus-map dump

#include <failsafe/logger.hh>
#include <neutrino/application.hh>
#include <sdlpp/app/entry_point.hh>

#include <ke/assets/registry.hh>
#include <ke/format/archive.hh>
#include <ke/assets/sprites.hh>
#include <ke/scenes/play_game_scene.hh>
#include <ke/resources/resources.hh>
#include <ke/resources/cell.hh> // TEMP: bonus-map dump

namespace {
    // The sprite-gallery debug scene needs room for names, so the app runs larger than the
    // game's 320x200 (switch these back with play_game_scene).
    constexpr int window_width = 320;
    constexpr int window_height = 200;

    // TEMP: dump each level's per-position bonus TYPE (attr>>2), so we can cross-reference our
    // decoded types against what the original game actually drops. Uses the production cell.hh
    // decode. Remove once the bonus table is corrected.
    void dump_bonus_map(const std::vector <rs::ke_level>& levels) {
        std::array <int, 32> global{};
        std::printf("\n===== KE BONUS MAP (type = attr>>2, decoded via cell.hh) =====\n");
        for (std::size_t li = 0; li < levels.size(); ++li) {
            const rs::ke_level& lvl = levels[li];
            std::array <int, 32> seen{};
            std::printf("\n--- Level %zu ---  (col x ->, row y v ; '##'=brick no-bonus, '..'=empty)\n    ", li + 1);
            for (int x = 0; x < rs::ke_level::cols; ++x) {
                std::printf("%3d", x);
            }
            std::printf("\n");
            for (int y = 0; y < rs::ke_level::rows; ++y) {
                std::printf("y%2d ", y);
                for (int x = 0; x < rs::ke_level::cols; ++x) {
                    const rs::ke_cell& c = lvl.at(x, y);
                    if (c.drops_bonus) {
                        const int t = static_cast <int>(c.bonus_type) & 31;
                        std::printf("%3d", t);
                        ++global[t];
                        seen[t] = 1;
                    } else if (!c.is_empty()) {
                        std::printf(" ##");
                    } else {
                        std::printf(" ..");
                    }
                }
                std::printf("\n");
            }
            std::printf("  types in this level: ");
            for (int t = 0; t < 32; ++t) {
                if (seen[t]) {
                    std::printf("%d[%.*s] ", t,
                                static_cast <int>(rs::bonus_name(static_cast <rs::bonus>(t)).size()),
                                rs::bonus_name(static_cast <rs::bonus>(t)).data());
                }
            }
            std::printf("\n");
        }
        std::printf("\n===== types across ALL levels (count) =====\n");
        for (int t = 0; t < 32; ++t) {
            if (global[t]) {
                std::printf("  type %2d  x%-4d  %.*s\n", t, global[t],
                            static_cast <int>(rs::bonus_name(static_cast <rs::bonus>(t)).size()),
                            rs::bonus_name(static_cast <rs::bonus>(t)).data());
            }
        }
        std::fflush(stdout);
    }
}

class ke : public neutrino::application {
    public:
        ke()
            : application(make_config()) {
            // Run the console at INFO and up. Warming the SFX cache opens dozens of audio
            // sources, each emitting a one-time resampling notice at DEBUG; INFO hides that
            // (and other debug/trace spam) by default. Lower it when debugging.
            failsafe::logger::set_min_level(LOGGER_LEVEL_INFO);
        }

    protected:
        // Default to the sprite gallery (built from m_res, loaded in ready()). Swap for
        // play_game_scene to run the game.
        std::unique_ptr <neutrino::base_scene> create_initial_scene() override {
            // TEMP (bonus-map dump): quit() only flags the app as stopping -- on_ready() still asks
            // for the initial scene afterwards. The dump path returns before set_ke_assets, so
            // building the gameplay scene here would throw "ke_assets has not been published" and
            // print a spurious scene-initialization error on the advertised dump-and-quit path.
            if (m_dump_only) {
                return nullptr;
            }
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
            if (std::getenv("KE_DUMP_BONUS")) { // TEMP: dump bonus map + quit
                dump_bonus_map(m_assets.levels);
                m_dump_only = true; // suppress create_initial_scene (see there)
                quit();
                return;
            }
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
        bool m_dump_only = false; // TEMP: KE_DUMP_BONUS ran; skip building the gameplay scene
};

SDLPP_MAIN(ke)
