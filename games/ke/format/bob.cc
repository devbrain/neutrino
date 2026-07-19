//
// BOB sprite decoder (flag 0x0305) -> onyx_image surface.
//

#include <ke/format/bob.hh>

#include <algorithm>
#include <cstring>
#include <utility>

namespace rs {
    namespace {
        constexpr std::size_t BOB_HEADER_SIZE = 28;
        constexpr std::uint16_t BOB_FLAGS = 0x0305;

        std::uint16_t read_u16le(const std::uint8_t* p) {
            return static_cast <std::uint16_t>(p[0]) | (static_cast <std::uint16_t>(p[1]) << 8);
        }

        std::int16_t read_s16le(const std::uint8_t* p) {
            return static_cast <std::int16_t>(read_u16le(p));
        }

        // Decompress a single colour plane (RLE, row-by-row). Output size is
        // row_bytes * height; skipped bytes retain the default value 0.
        bool decode_plane(const std::uint8_t* start, const std::uint8_t* end,
                          int width, int height,
                          std::vector <std::uint8_t>& out, std::string& error) {
            const std::uint32_t row_bytes = (static_cast <std::uint32_t>(width) + 3u) / 4u;
            out.assign(static_cast <std::size_t>(row_bytes) * static_cast <std::size_t>(height), 0);

            const std::uint8_t* p = start;
            for (int y = 0; y < height; ++y) {
                if (p >= end) {
                    error = "BOB: EOF reading plane op_count";
                    return false;
                }
                const std::uint8_t op_count = *p++;
                std::uint32_t x = 0;
                for (std::uint8_t i = 0; i < op_count; ++i) {
                    if (p >= end) {
                        error = "BOB: EOF reading plane op";
                        return false;
                    }
                    const auto n = static_cast <std::int8_t>(*p++);
                    if (n < 0) {
                        x += static_cast <std::uint32_t>(-n);
                    } else if (n > 0) {
                        if (x + static_cast <std::uint32_t>(n) > row_bytes) {
                            error = "BOB: plane row overflow";
                            return false;
                        }
                        if (p + n > end) {
                            error = "BOB: EOF reading plane literal bytes";
                            return false;
                        }
                        std::memcpy(out.data() + static_cast <std::size_t>(y) * row_bytes + x, p,
                                    static_cast <std::size_t>(n));
                        p += n;
                        x += static_cast <std::uint32_t>(n);
                    }
                    if (x > row_bytes) {
                        error = "BOB: plane row overflow (post-op)";
                        return false;
                    }
                }
            }
            return true;
        }

        // Decompress the transparency mask (RLE, row-by-row). Output size is
        // width * height; 0 = transparent, 255 = opaque.
        bool decode_mask(const std::uint8_t* start, const std::uint8_t* end,
                         int width, int height,
                         std::vector <std::uint8_t>& out, std::string& error) {
            out.assign(static_cast <std::size_t>(width) * static_cast <std::size_t>(height), 0);

            const std::uint8_t* p = start;
            for (int y = 0; y < height; ++y) {
                if (p >= end) {
                    error = "BOB: EOF reading mask packet count";
                    return false;
                }
                const std::uint8_t pkt = *p++;
                int x = 0;
                for (std::uint8_t i = 0; i < pkt; ++i) {
                    if (p >= end) {
                        error = "BOB: EOF reading mask run";
                        return false;
                    }
                    const auto run = static_cast <std::int8_t>(*p++);
                    if (run < 0) {
                        x += -run;
                    } else if (run > 0) {
                        if (x + run > width) {
                            error = "BOB: mask row overflow";
                            return false;
                        }
                        std::memset(out.data() + static_cast <std::size_t>(y) * width + x, 255,
                                    static_cast <std::size_t>(run));
                        x += run;
                    }
                    if (x > width) {
                        error = "BOB: mask row overflow (post-op)";
                        return false;
                    }
                }
            }
            return true;
        }
    } // namespace

    bool bob_image::parse(std::span <const std::uint8_t> data, std::string& error) {
        blocks_.clear();
        frames_.clear();
        data_.assign(data.begin(), data.end());

        if (data_.size() < 2) {
            error = "BOB: resource too small";
            return false;
        }

        const std::uint16_t block_count = read_u16le(data_.data());
        std::size_t pos = 2;

        for (std::uint16_t bi = 0; bi < block_count; ++bi) {
            if (pos + 2 > data_.size()) break;
            const std::uint16_t bs = read_u16le(data_.data() + pos);
            if (bs < BOB_HEADER_SIZE || pos + bs > data_.size()) {
                error = "BOB: block " + std::to_string(bi) + " has invalid size";
                break;
            }

            const std::uint8_t* block = data_.data() + pos;
            bob_frame f;
            f.width = read_u16le(block + 2);
            f.height = read_u16le(block + 4);
            const std::uint16_t header_size = read_u16le(block + 6);
            const std::uint16_t flags = read_u16le(block + 8);
            f.x_off_a = read_s16le(block + 10);
            f.y_off_a = read_s16le(block + 12);
            f.x_off_b = read_s16le(block + 14);
            f.y_off_b = read_s16le(block + 16);

            if (flags == BOB_FLAGS && header_size == BOB_HEADER_SIZE && f.width > 0 && f.height > 0) {
                blocks_.emplace_back(pos, bs);
                frames_.push_back(f);
            } else {
                error = "BOB: block " + std::to_string(bi) + " has unsupported header";
                break;
            }
            pos += bs;
        }

        if (frames_.empty()) {
            if (error.empty()) error = "BOB: no valid frames";
            return false;
        }
        return true;
    }

    bool bob_image::decode_frame_rgba(std::size_t index,
                                      sdlpp::const_palette_ref palette,
                                      std::vector <std::uint8_t>& rgba,
                                      int& width, int& height,
                                      std::string& error) const {
        if (index >= blocks_.size()) {
            error = "BOB: frame index out of range";
            return false;
        }
        if (!palette || palette.size() < 256) {
            error = "BOB: palette must have 256 colours";
            return false;
        }

        const auto [offset, size] = blocks_[index];
        const std::uint8_t* block = data_.data() + offset;
        const std::uint8_t* block_end = block + size;

        width = read_u16le(block + 2);
        height = read_u16le(block + 4);
        const std::uint16_t header_size = read_u16le(block + 6);

        std::uint16_t plane_off[5];
        for (int i = 0; i < 5; ++i) plane_off[i] = read_u16le(block + 18 + i * 2);

        const std::uint8_t* base = block + header_size;

        // Validate plane offsets are monotonic and within the block.
        for (int i = 0; i < 4; ++i) {
            if (plane_off[i] > plane_off[i + 1]) {
                error = "BOB: plane offsets not monotonic";
                return false;
            }
        }
        if (base + plane_off[4] > block_end) {
            error = "BOB: mask offset out of range";
            return false;
        }

        std::vector <std::uint8_t> planes[4];
        for (int p = 0; p < 4; ++p) {
            const std::uint8_t* ps = base + plane_off[p];
            const std::uint8_t* pe = base + plane_off[p + 1];
            if (ps > block_end || pe > block_end || ps > pe) {
                error = "BOB: plane range out of bounds";
                return false;
            }
            if (!decode_plane(ps, pe, width, height, planes[p], error)) return false;
        }

        std::vector <std::uint8_t> alpha;
        if (!decode_mask(base + plane_off[4], block_end, width, height, alpha, error)) return false;

        const std::uint32_t row_bytes = (static_cast <std::uint32_t>(width) + 3u) / 4u;
        rgba.assign(static_cast <std::size_t>(width) * static_cast <std::size_t>(height) * 4, 0);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const std::uint32_t plane = static_cast <std::uint32_t>(x) & 3u;
                const std::uint32_t idx = static_cast <std::uint32_t>(y) * row_bytes
                                          + (static_cast <std::uint32_t>(x) >> 2);
                const std::uint8_t pal_idx = planes[plane][idx];
                const auto color = palette.get_color(pal_idx);
                std::uint8_t* dst = rgba.data() + (static_cast <std::size_t>(y) * width + x) * 4;
                dst[0] = color.r;
                dst[1] = color.g;
                dst[2] = color.b;
                dst[3] = alpha[static_cast <std::size_t>(y) * width + x];
            }
        }
        return true;
    }

    bool bob_image::decode(std::size_t index,
                           sdlpp::const_palette_ref palette,
                           onyx_image::surface& surf,
                           std::string& error) const {
        std::vector <std::uint8_t> rgba;
        int width = 0, height = 0;
        if (!decode_frame_rgba(index, palette, rgba, width, height, error)) return false;

        if (!surf.set_size(width, height, onyx_image::pixel_format::rgba8888)) {
            error = "BOB: failed to allocate surface";
            return false;
        }
        for (int y = 0; y < height; ++y) {
            surf.write_pixels(0, y, width * 4, rgba.data() + static_cast <std::size_t>(y) * width * 4);
        }
        return true;
    }

    bool bob_image::decode_atlas(sdlpp::const_palette_ref palette,
                                 onyx_image::surface& surf,
                                 std::string& error,
                                 std::vector<neutrino::rect>& rects,
                                 int max_width,
                                 int padding) const {
        // Decode every frame and compute a shelf-packed layout.
        struct placed {
            std::vector <std::uint8_t> rgba;
            int w{}, h{}, x{}, y{};
        };
        std::vector <placed> tiles;
        tiles.reserve(frames_.size());
        rects.reserve(frames_.size());

        int cursor_x = 0, cursor_y = 0, row_height = 0, atlas_width = 0;
        for (std::size_t i = 0; i < blocks_.size(); ++i) {
            placed t;
            if (!decode_frame_rgba(i, palette, t.rgba, t.w, t.h, error)) return false;

            // Wrap to a new shelf when this tile would overflow the width.
            if (cursor_x > 0 && cursor_x + t.w > max_width) {
                cursor_y += row_height + padding;
                cursor_x = 0;
                row_height = 0;
            }
            t.x = cursor_x;
            t.y = cursor_y;
            cursor_x += t.w + padding;
            row_height = std::max(row_height, t.h);
            atlas_width = std::max(atlas_width, t.x + t.w);
            tiles.push_back(std::move(t));
            rects.emplace_back(t.x, t.y, t.w, t.h);
        }

        const int atlas_height = cursor_y + row_height;
        if (atlas_width <= 0 || atlas_height <= 0) {
            error = "BOB: empty atlas";
            return false;
        }

        if (!surf.set_size(atlas_width, atlas_height, onyx_image::pixel_format::rgba8888)) {
            error = "BOB: failed to allocate atlas surface";
            return false;
        }

        // Blit each tile and record its rectangle.
        for (std::size_t i = 0; i < tiles.size(); ++i) {
            const placed& t = tiles[i];
            for (int y = 0; y < t.h; ++y) {
                surf.write_pixels(t.x * 4, t.y + y, t.w * 4,
                                  t.rgba.data() + static_cast <std::size_t>(y) * t.w * 4);
            }
            onyx_image::subrect sr;
            sr.rect = {t.x, t.y, t.w, t.h};
            sr.kind = onyx_image::subrect_kind::frame;
            sr.user_tag = static_cast <std::uint32_t>(i);
            surf.set_subrect(static_cast <int>(i), sr);
        }

        return true;
    }

    sdlpp::palette bob_grayscale_palette() {
        auto pal = sdlpp::palette::create_grayscale(8);
        if (!pal) {
            return {};
        }
        return std::move(*pal);
    }
} // namespace ke
