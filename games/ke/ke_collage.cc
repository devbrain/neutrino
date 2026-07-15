//
// Debug tool: render a labeled contact sheet of KE_SPELL and KE_NMY, each frame with its
// block number written below in the BIOS 8x8 font. For verifying the frame->number tables
// (bonus_capsule / enemy_anim). Writes ke_spell.png and ke_nmy.png in the working directory.
//

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>

#include <onyx_font/bios_font.hh>
#include <onyx_image/codecs/png.hpp>
#include <onyx_image/surface.hpp>

#include "resources/ke_loader.hh"

namespace {
    struct rgba {
        std::uint8_t r, g, b, a;
    };

    void put_pixel(onyx_image::memory_surface& s, int x, int y, rgba c) {
        if (x < 0 || y < 0 || x >= s.width() || y >= s.height()) {
            return;
        }
        const auto px = s.mutable_pixels();
        const std::size_t i = (static_cast <std::size_t>(y) * s.width() + x) * 4;
        px[i + 0] = c.r;
        px[i + 1] = c.g;
        px[i + 2] = c.b;
        px[i + 3] = c.a;
    }

    void fill(onyx_image::memory_surface& s, rgba c) {
        for (int y = 0; y < s.height(); ++y) {
            for (int x = 0; x < s.width(); ++x) {
                put_pixel(s, x, y, c);
            }
        }
    }

    // Draw text with the embedded BIOS 8x8 font (1 bit/pixel glyphs).
    void draw_text(onyx_image::memory_surface& s, int x, int y, std::string_view text, rgba c) {
        const auto& font = onyx_font::bios_font_8x8();
        int pen = x;
        for (const char ch : text) {
            const auto g = font.get_glyph(static_cast <std::uint8_t>(ch));
            for (std::uint16_t gy = 0; gy < g.height(); ++gy) {
                for (std::uint16_t gx = 0; gx < g.width(); ++gx) {
                    if (g.pixel(gx, gy)) {
                        put_pixel(s, pen + gx, y + gy, c);
                    }
                }
            }
            pen += static_cast <int>(g.width());
        }
    }

    // Copy one frame (from the RGBA atlas) into the collage, skipping transparent texels.
    void blit_frame(onyx_image::memory_surface& dst, int dx, int dy,
                    const sdlpp::surface& atlas, const neutrino::rect& r) {
        SDL_Surface* s = atlas.get();
        for (int yy = 0; yy < r.h; ++yy) {
            for (int xx = 0; xx < r.w; ++xx) {
                std::uint8_t cr, cg, cb, ca;
                if (!SDL_ReadSurfacePixel(s, r.x + xx, r.y + yy, &cr, &cg, &cb, &ca) || ca == 0) {
                    continue;
                }
                put_pixel(dst, dx + xx, dy + yy, {cr, cg, cb, 255});
            }
        }
    }

    bool make_collage(const rs::tile_sheet_def& sheet, const std::filesystem::path& out) {
        const std::size_t n = sheet.source_rects.size();
        if (n == 0) {
            return false;
        }
        int max_w = 0;
        int max_h = 0;
        for (const auto& r : sheet.source_rects) {
            max_w = std::max(max_w, r.w);
            max_h = std::max(max_h, r.h);
        }

        constexpr int cols = 10;
        constexpr int pad = 4;
        constexpr int gap = 2;
        constexpr int label_h = 8;
        const int cell_w = max_w + 2 * pad;
        const int cell_h = max_h + gap + label_h + 2 * pad;
        const int rows = static_cast <int>((n + cols - 1) / cols);

        onyx_image::memory_surface collage;
        if (!collage.set_size(cols * cell_w, rows * cell_h, onyx_image::pixel_format::rgba8888)) {
            return false;
        }
        fill(collage, {32, 32, 40, 255});

        for (std::size_t i = 0; i < n; ++i) {
            const int col = static_cast <int>(i % cols);
            const int row = static_cast <int>(i / cols);
            const int cx = col * cell_w;
            const int cy = row * cell_h;
            const auto& r = sheet.source_rects[i];

            blit_frame(collage, cx + pad + (max_w - r.w) / 2, cy + pad, sheet.image, r);

            const std::string num = std::to_string(i);
            const int tw = static_cast <int>(num.size()) * 8;
            draw_text(collage, cx + (cell_w - tw) / 2, cy + pad + max_h + gap, num, {255, 235, 60, 255});
        }

        if (!onyx_image::save_png(collage, out)) {
            std::cerr << "failed to save " << out << "\n";
            return false;
        }
        std::cerr << "wrote " << out << " (" << n << " frames, "
                  << collage.width() << "x" << collage.height() << ")\n";
        return true;
    }
}

int main(int argc, char** argv) {
    const char* path = argc > 1 ? argv[1] : "/home/igor/games/ke/Krypton-Egg_DOS_EN/ke.rsc";

    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        std::cerr << "cannot open " << path << "\n";
        return 1;
    }
    auto res = rs::parse(ifs);
    if (!res) {
        std::cerr << "failed to parse resources\n";
        return 1;
    }

    bool ok = true;
    for (const auto* name : {"ke_spell", "ke_nmy"}) {
        const auto it = res->tile_sheets.find(name);
        if (it == res->tile_sheets.end()) {
            std::cerr << "no " << name << " sheet\n";
            ok = false;
            continue;
        }
        ok &= make_collage(it->second, std::string(name) + ".png");
    }
    return ok ? 0 : 1;
}
