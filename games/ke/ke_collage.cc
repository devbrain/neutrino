//
// Debug tool: dump every image resource in a KE archive to PNG in the working directory.
//   * each BOB sheet  -> a labeled contact sheet <base>.png (every frame with its block
//     number below it in the BIOS 8x8 font; for verifying the frame->number tables);
//   * each GIF        -> a plain decoded <base>.png.
//
// Uses only the format layer (ke_format): it reads the archive directory and decodes the
// BOB sheets directly, without the engine-facing resource container. GIFs are decoded with
// onyx_image's own gif codec.
//

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

#include <onyx_font/bios_font.hh>
#include <onyx_image/codecs/gif.hpp>
#include <onyx_image/codecs/png.hpp>
#include <onyx_image/surface.hpp>
#include <onyx_image/types.hpp>

#include <sdlpp/video/color.hh>
#include <sdlpp/video/palette.hh>
#include <sdlpp/video/surface.hh>
#include <sdlpp/image/sdl_surface_adapter.hh>

#include <neutrino/video/geometry_types.hh>

#include <ke/format/archive.hh>
#include <ke/format/bob.hh>

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

    // Read a resource's raw bytes from the archive stream.
    std::vector <std::uint8_t> read_bytes(std::istream& is, const rs::resource& r) {
        std::vector <std::uint8_t> data(r.size);
        is.seekg(r.offset, std::ios::beg);
        is.read(reinterpret_cast <std::istream::char_type*>(data.data()), r.size);
        return data;
    }

    // Build a 256-colour SDL palette from a KE .pal resource (256 RGB triples).
    std::optional <sdlpp::palette> palette_from_rgb(std::span <const std::uint8_t> rgb) {
        if (rgb.size() < 256 * 3) {
            return std::nullopt;
        }
        std::vector <sdlpp::color> colors;
        colors.reserve(256);
        for (std::size_t i = 0; i < 256; ++i) {
            colors.emplace_back(rgb[i * 3 + 0], rgb[i * 3 + 1], rgb[i * 3 + 2], 255);
        }
        auto pal = sdlpp::palette::create(256);
        if (!pal) {
            return std::nullopt;
        }
        if (!pal->set_colors(colors)) {
            return std::nullopt;
        }
        return std::move(*pal);
    }

    bool make_collage(const sdlpp::surface& image, const std::vector <neutrino::rect>& rects,
                      const std::filesystem::path& out) {
        const std::size_t n = rects.size();
        if (n == 0) {
            return false;
        }
        int max_w = 0;
        int max_h = 0;
        for (const auto& r : rects) {
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
            const auto& r = rects[i];

            blit_frame(collage, cx + pad + (max_w - r.w) / 2, cy + pad, image, r);

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

    const rs::load_result lr = rs::load_resource(ifs);
    for (const auto& d : lr.diagnostics) {
        std::cerr << (d.severity == rs::diagnostic::severity_level::error ? "error: " : "warning: ")
                  << d.message << "\n";
    }
    if (lr.has_errors()) {
        return 1;
    }

    const auto find = [&](std::string_view name) -> const rs::resource* {
        for (const auto& r : lr.resources) {
            if (r.name == name) {
                return &r;
            }
        }
        return nullptr;
    };

    // Palette for a sheet: sibling <base>.pal, else the gameplay palette, else grayscale.
    const auto palette_for = [&](const std::string& base) -> sdlpp::palette {
        for (const std::string& cand : {base + ".pal", std::string("ke_play.pal")}) {
            if (const auto* pr = find(cand)) {
                if (auto p = palette_from_rgb(read_bytes(ifs, *pr))) {
                    return std::move(*p);
                }
            }
        }
        return rs::bob_grayscale_palette();
    };

    bool ok = true;
    for (const auto& r : lr.resources) {
        if (r.name.ends_with(".bob")) {
            // BOB sprite sheet -> labeled contact sheet.
            const std::string base = r.name.substr(0, r.name.size() - 4);

            rs::bob_image bob;
            std::string err;
            if (const auto data = read_bytes(ifs, r); !bob.parse(data, err)) {
                std::cerr << "parse " << r.name << ": " << err << "\n";
                ok = false;
                continue;
            }

            sdlpp::surface image;
            sdlpp::image::sdl_surface_adapter adaptor(image);
            std::vector <neutrino::rect> rects;
            if (const sdlpp::palette pal = palette_for(base);
                !bob.decode_atlas(pal, adaptor, err, rects)) {
                std::cerr << "decode " << r.name << ": " << err << "\n";
                ok = false;
                continue;
            }

            // Keep the source extension in the output name: the archive has both a
            // <base>.bob and a <base>.gif for some names (ke_menu, ke_monst), so
            // <base>.png alone would collide.
            ok &= make_collage(image, rects, r.name + ".png");
        } else if (r.name.ends_with(".gif")) {
            // GIF -> plain PNG.
            const auto data = read_bytes(ifs, r);
            onyx_image::memory_surface surf;
            onyx_image::decode_options opts;
            opts.output = onyx_image::color_output::rgba;
            if (const auto dr = onyx_image::gif_decoder::decode(data, surf, opts); !dr) {
                std::cerr << "decode " << r.name << ": " << dr.message << "\n";
                ok = false;
                continue;
            }
            const std::filesystem::path out = r.name + ".png";
            if (!onyx_image::save_png(surf, out)) {
                std::cerr << "failed to save " << out << "\n";
                ok = false;
                continue;
            }
            std::cerr << "wrote " << out << " (" << surf.width() << "x" << surf.height() << ")\n";
        }
    }
    return ok ? 0 : 1;
}
