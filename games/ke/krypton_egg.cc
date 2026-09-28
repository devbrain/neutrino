//
// Created by igor on 12/07/2026.
//

#include <fstream>
#include <memory>
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

    // Dump each level's runtime bonus IDs ((attr>>2)-1) using the production decoder.
    // KE_DUMP_BONUS provides a visual grid to cross-check with docs/bonuses.md.
    void dump_bonus_map(const std::vector <rs::ke_level>& levels) {
        std::array <int, 32> global{};
        std::printf("\n===== KE BONUS MAP (ID = (attr>>2)-1, decoded via cell.hh) =====\n");
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
                        const int t = static_cast <int>(c.bonus_type);
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
            // quit() only FLAGS the app as stopping -- on_ready() still asks for the initial scene
            // afterwards. So every path where ready() bailed out early reaches here: a resource
            // file that would not open, a parse failure, an archive with no levels, and the
            // bonus-map dump. In all of them m_assets was never built, and the gameplay scene
            // takes it by reference -- dereferencing the empty pointer to bind that reference is
            // undefined behaviour, on the most ordinary failure a player can hit.
            //
            // Gate on the assets themselves rather than on a separate "did we bail" flag: the
            // pointer IS the record of whether ready() got far enough, so the two cannot drift.
            if (!m_assets) {
                return nullptr; // ready() already logged why and asked to quit
            }
            return std::make_unique <play_game_scene>(*m_assets);
        }

        void on_config(int argc, char* argv[]) override {
            if (argc == 2) {
                m_path_to_rs = argv[1];
            }
        }

        void ready() override {
            constexpr auto* ke_rsc_default = "/home/igor/games/ke/Krypton-Egg_DOS_EN/ke.rsc";
            const std::string path = m_path_to_rs.empty() ? ke_rsc_default : m_path_to_rs;
            std::ifstream ifs(path.c_str(), std::ios::binary);
            if (!ifs) {
                // The path that was actually tried, not the default -- naming the built-in path
                // while a command-line one failed sends the reader after the wrong file.
                LOG_ERROR("ke: cannot open", path);
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
            if (std::getenv("KE_DUMP_BONUS")) { // TEMP: dump bonus map + quit
                dump_bonus_map(m_res.levels);
                quit(); // m_assets stays empty -> create_initial_scene builds nothing
                return;
            }
            m_assets = std::make_unique <rs::ke_assets>();
            m_assets->levels = std::move(m_res.levels);
            rs::set_ke_assets(*m_assets);
            rs::define_sprites(m_res);
            m_assets->m_resources = &m_res;
        }

        // The counterpart to ready(): release the assets HERE, not by letting the member die in
        // ~ke(). ke_assets owns a sprite_cache holding GPU textures, and ~ke() runs after the
        // base class has already taken the renderer down -- releasing them there frees texture
        // handles against a dead device. This hook runs while the renderer is still alive.
        void teardown() override {
            if (m_assets) {
                rs::release_sprites(); // unregister the animations that reference the sheets
                rs::clear_ke_assets(); // unpublish before the storage goes
                m_assets.reset();
            }
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
        // Owned here for the whole run, published via set_ke_assets and injected into the
        // gameplay scene. Indirect so teardown() can release it at the right moment -- ke_assets
        // is non-copyable/non-movable (it holds a live sprite_cache), so a by-value member could
        // only be destroyed with the application itself, which is too late.
        std::unique_ptr <rs::ke_assets> m_assets;
};

SDLPP_MAIN(ke)
