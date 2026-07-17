//
// Created by igor on 17/07/2026.
//

#include <failsafe/logger.hh>
#include <neutrino/video/image_loader.hh>

#include "game_resources.hh"
#include "ke_loader.hh"
#include "ke_bob.hh"

namespace rs {
    namespace {
        // Read a resource's raw bytes from the archive stream.
        std::vector <std::uint8_t> read_resource(std::istream& is, const resource& r) {
            std::vector <std::uint8_t> data(r.size);
            is.seekg(r.offset, std::ios::beg);
            is.read(reinterpret_cast <std::istream::char_type*>(data.data()), r.size);
            return data;
        }

        std::optional <sdlpp::palette> palette_from_colors(std::span <const sdlpp::color> colors) {
            if (colors.size() < 256) {
                return std::nullopt;
            }

            auto pal = sdlpp::palette::create(256);
            if (!pal) {
                return std::nullopt;
            }
            if (!pal->set_colors(colors.first(256))) {
                return std::nullopt;
            }
            return std::move(*pal);
        }

        std::optional <sdlpp::palette> palette_from_rgb_bytes(std::span <const std::uint8_t> rgb) {
            if (rgb.size() < 256 * 3) {
                return std::nullopt;
            }

            std::vector <sdlpp::color> colors;
            colors.reserve(256);
            for (std::size_t i = 0; i < 256; ++i) {
                colors.emplace_back(
                    rgb[i * 3 + 0],
                    rgb[i * 3 + 1],
                    rgb[i * 3 + 2],
                    255);
            }
            return palette_from_colors(colors);
        }
    }

    std::optional <game_resources> parse(std::istream& is) {
        auto result = load_resource(is);
        if (result.has_errors()) {
            for (const auto& diag : result.diagnostics) {
                if (diag.severity == diagnostic::severity_level::error) {
                    LOG_ERROR(diag.message);
                } else {
                    LOG_WARN(diag.message);
                }
            }
            return std::nullopt;
        }

        game_resources gr;
        std::map <std::string, sdlpp::palette> pals;
        for (const auto& r : result.resources) {
            if (r.name.find(".gif") != std::string::npos) {
                auto data = read_resource(is, r);
                gr.backdrops.emplace(r.name, neutrino::cpu_texture_atlas(neutrino::load_image(data)));
            }
            if (r.name.find(".pal") != std::string::npos) {
                auto data = read_resource(is, r);
                if (auto pal = palette_from_rgb_bytes(data)) {
                    pals.emplace(r.name, std::move(*pal));
                }
            }
        }

        for (const auto& r : result.resources) {
            if (auto idx = r.name.find(".bob"); idx != std::string::npos) {
                auto data = read_resource(is, r);
                bob_image bob;

                if (std::string err; !bob.parse(data, err)) {
                    LOG_ERROR("Error while parsing", r.name, ":", err);
                    return std::nullopt;
                }
                tile_sheet_def tl_def;

                sdlpp::image::sdl_surface_adapter adaptor(tl_def.image);
                std::string err;
                auto base = r.name.substr(0, idx);
                bool bob_rc = false;

                if (auto itr = pals.find(base + ".pal"); itr != pals.end()) {
                    // 1. sibling <base>.pal
                    bob_rc = bob.decode_atlas(itr->second, adaptor, err, tl_def.source_rects);
                } else {
                    if (auto itr_gif = gr.backdrops.find(base + ".gif"); itr_gif != gr.backdrops.end()) {
                        // 2. sibling <base>.gif global colour table (the GIF decoder yields indexed8 + palette)
                        const auto& gif_srf = itr_gif->second.surface();
                        if (gif_srf.format() == sdlpp::pixel_format_enum::INDEX8) {
                            bob_rc = bob.decode_atlas(gif_srf.get_palette(), adaptor, err, tl_def.source_rects);
                        }
                    } else {
                        if (auto itr_play = pals.find("ke_play.pal"); itr_play != pals.end()) {
                            // 3. gameplay palette
                            bob_rc = bob.decode_atlas(itr_play->second, adaptor, err, tl_def.source_rects);
                        } else {
                            bob_rc = bob.decode_atlas(bob_grayscale_palette(), adaptor, err, tl_def.source_rects);
                        }
                    }
                }
                if (!bob_rc) {
                    LOG_ERROR("Error while decoding", r.name, ":", err);
                    return std::nullopt;
                }
                tl_def.origins.reserve(bob.frame_count());
                for (std::size_t i = 0; i < bob.frame_count(); ++i) {
                    const bob_frame& f = bob.frame(i);
                    tl_def.origins.emplace_back(f.x_off_a, f.y_off_a);
                }

                gr.tile_sheets.emplace(base, std::move(tl_def));
            }
        }
        for (const auto& r : result.resources) {
            if (r.name.find(".tab") != std::string::npos) {
                const auto data = read_resource(is, r);
                tab_levels tab;
                std::string err;
                if (!tab.parse(data, err)) {
                    LOG_ERROR("Error while parsing TAB in ", r.name, ":", err);
                    return std::nullopt;
                }
                gr.levels.reserve(tab.levels.size());
                for (const auto& lvl : tab.levels) {
                    gr.levels.emplace_back(ke_level::decode(lvl));
                }
            }
        }
        LOG_DEBUG("Loaded ", gr.backdrops.size(), "backdrops");
        LOG_DEBUG("Loaded ", pals.size(), "palettes");
        return gr;
    }
}
