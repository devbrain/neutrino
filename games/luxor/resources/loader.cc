//
// Created by igor on 14/10/2026.
//
#include <neutrino/video/image_loader.hh>

#include "loader.hh"
#include <luxor/resources/parser/spr_parser.hh>

namespace luxor::rc {
    expected <void, std::string> apply_alpha(sdlpp::surface& img, const sdlpp::surface& alpha) {
        const auto w = img.width();
        const auto h = img.height();
        if (w != alpha.width() || h != alpha.height()) {
            return unexpected("Surface dimensions mismatch: img is " +
                              std::to_string(w) + "x" + std::to_string(h) +
                              ", alpha is " + std::to_string(alpha.width()) + "x" +
                              std::to_string(alpha.height()));
        }

        if (w == 0 || h == 0) {
            return {};
        }

        // Ensure destination image is ABGR8888 (standard 32-bit RGBA)
        if (img.format() != sdlpp::pixel_format_enum::ABGR8888) {
            auto converted = img.convert(sdlpp::pixel_format_enum::ABGR8888);
            if (!converted) {
                return unexpected("Failed to convert image to ABGR8888: " + converted.error());
            }
            img = std::move(*converted);
        }

        sdlpp::surface::lock_guard lock_i(img);
        sdlpp::surface::lock_guard lock_a(const_cast <sdlpp::surface&>(alpha));

        auto img_format = sdlpp::pixel_format(img.format());
        const uint32_t a_mask = img_format.details->Amask;
        const uint8_t a_shift = img_format.details->Ashift;

        auto* img_raw = static_cast <uint8_t*>(img.get_pixels());
        const auto img_pitch = img.get_pitch();

        // Fast-path: both surfaces are 32-bit ABGR8888
        if (alpha.format() == sdlpp::pixel_format_enum::ABGR8888) {
            auto alpha_format = sdlpp::pixel_format(alpha.format());
            const uint8_t a_s = alpha_format.details->Ashift;
            const uint8_t r_s = alpha_format.details->Rshift;
            const uint8_t g_s = alpha_format.details->Gshift;

            const auto* alpha_raw = static_cast <const uint8_t*>(alpha.get_pixels());
            const auto alpha_pitch = alpha.get_pitch();

            for (int y = 0; y < static_cast <int>(h); ++y) {
                auto* dst_row = reinterpret_cast <uint32_t*>(img_raw + (y * img_pitch));
                const auto* src_row = reinterpret_cast <const uint32_t*>(alpha_raw + (y * alpha_pitch));

                for (int x = 0; x < static_cast <int>(w); ++x) {
                    const uint32_t px_a = src_row[x];
                    const auto r = static_cast <uint8_t>((px_a >> r_s) & 0xFF);
                    const auto g = static_cast <uint8_t>((px_a >> g_s) & 0xFF);
                    const auto a_raw = static_cast <uint8_t>((px_a >> a_s) & 0xFF);

                    const uint8_t a = (a_raw < 255 || r != g) ? a_raw : r;
                    dst_row[x] = (dst_row[x] & ~a_mask) | ((static_cast <uint32_t>(a) << a_shift) & a_mask);
                }
            }
            return {};
        }

        // Fallback path: alpha surface is in another format (e.g. RGB24, INDEX8)
        auto alpha_get_px = alpha.get_get_pixel_fast();
        const auto* alpha_px = alpha.get_pixels();
        const auto alpha_pitch = alpha.get_pitch();
        auto alpha_format = sdlpp::pixel_format(alpha.format());

        if (!alpha_get_px || !alpha_px) {
            return unexpected("Failed to get pixel accessor for alpha surface");
        }

        for (int y = 0; y < static_cast <int>(h); ++y) {
            auto* dst_row = reinterpret_cast <uint32_t*>(img_raw + (y * img_pitch));
            for (int x = 0; x < static_cast <int>(w); ++x) {
                auto px_a = alpha_get_px(alpha_px, alpha_pitch, x, y);
                sdlpp::color col_a = alpha_format.get_rgba(px_a);

                const uint8_t a = (col_a.a < 255 || col_a.r != col_a.g) ? col_a.a : col_a.r;
                dst_row[x] = (dst_row[x] & ~a_mask) | ((static_cast <uint32_t>(a) << a_shift) & a_mask);
            }
        }

        return {};
    }

    sdlpp::surface load_image(const vfs& fs, const std::string& path) {
        auto img = fs.read(path);
        if (img.empty()) {
            return {};
        }
        try {
            return neutrino::load_image(img);
        } catch (...) {
            return {};
        }
    }

    expected <neutrino::sprite_def, std::string> load_sprite(const vfs& fs, const std::string& path) {
        auto txt = fs.read(path);

        if (const auto sprite_file = parse_spr(txt.data(), txt.size())) {
            const auto& sprite = *sprite_file;
            auto img = load_image(fs, sprite.diffuse_texture);
            if (!img) {
                return unexpected ("Failed to load diffused texture " + sprite.diffuse_texture);
            }
            if (sprite.has_alpha && !sprite.alpha_texture.empty() && sprite.alpha_texture != "none") {
                auto alpha = load_image(fs, sprite_file->alpha_texture);
                if (!alpha) {
                    return unexpected <std::string>("Failed to load alpha texture " + sprite.alpha_texture);
                }
                if (auto err = apply_alpha(img, alpha); !err) {
                    return unexpected ("Failed to apply alpha " + sprite.alpha_texture + " " + err.error());
                }
            }
            neutrino::sprite_def_builder b;
            return b.from_surface(std::move(img)).with_grid(sprite.grid).build();
        }
        return unexpected ("failed to parse file " + path);
    }

    expected<font_file, std::string> load_font(const vfs& fs, const std::string& path) {
        auto data = fs.read(path);
        if (data.empty()) {
            return unexpected("Failed to read font file or file empty: " + path);
        }
        return parse_font(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
    }

    expected<depth_table, std::string> load_depths(const vfs& fs, const std::string& path) {
        auto data = fs.read(path);
        if (data.empty()) {
            return unexpected("Failed to read depths file or file empty: " + path);
        }
        return parse_depths(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
    }

    expected<string_list, std::string> load_string_list(const vfs& fs, const std::string& path) {
        auto data = fs.read(path);
        if (data.empty()) {
            return unexpected("Failed to read string list file or file empty: " + path);
        }
        return parse_string_list(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
    }

    expected<config_document, std::string> load_config(const vfs& fs, const std::string& path) {
        auto data = fs.read(path);
        if (data.empty()) {
            return unexpected("Failed to read config file or file empty: " + path);
        }
        return parse_config(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
    }

    expected<level_file, std::string> load_level(const vfs& fs, const std::string& path) {
        return load_config(fs, path);
    }

    expected<config_document, std::string> load_psys(const vfs& fs, const std::string& path) {
        return load_config(fs, path);
    }

    expected<tinyobj::ObjReader, std::string> load_obj(const vfs& fs, const std::string& path) {
        auto data = fs.read(path);
        if (data.empty()) {
            return unexpected("Failed to read OBJ file or file empty: " + path);
        }
        std::string text(data.begin(), data.end());
        tinyobj::ObjReaderConfig reader_config;
        tinyobj::ObjReader reader;
        if (!reader.ParseFromString(text, "", reader_config)) {
            return unexpected("Failed to parse OBJ file: " + reader.Error());
        }
        return reader;
    }

    expected<sl3_file, std::string> load_sl3(const vfs& fs, const std::string& path) {
        auto data = fs.read(path);
        if (data.empty()) {
            return unexpected("Failed to read SL3 file or file empty: " + path);
        }
        return parse_sl3(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()));
    }
}
