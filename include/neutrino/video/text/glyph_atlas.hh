//
// Created by igor on 05/11/2026.
//

#pragma once

#include <optional>
#include <span>
#include <unordered_set>
#include <vector>
#include <limits>
#include <algorithm>
#include <cmath>
#include <cstdint>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/atlas/bitmask.hh>
#include <neutrino/video/atlas/image_atlas.hh>
#include <neutrino/video/image/image_bank.hh>
#include <neutrino/video/image/image_builder.hh>
#include <neutrino/video/color.hh>
#include <neutrino/video/palette.hh>

#include <failsafe/enforce.hh>

namespace neutrino {
    namespace details {
        struct glyph_atlas_access;
    }

    /**
     * @brief Metrics of one glyph (font_plan.md F6, P5).
     *
     * Coordinates: x right, y down. The decoder states only @ref glyph and @ref advance (in
     * `start_glyph`); `end_glyph` derives the rest from the pixels written, which are given
     * relative to the pen on the baseline.
     */
    struct NEUTRINO_EXPORT glyph_properties {
        /// The game's id for the glyph (character code, glyph number); the image bank entry id.
        std::uint32_t glyph = 0;

        /// How far the pen moves right after drawing the glyph, in pixels. May be fractional
        /// (scalable fonts); kept to the nearest 1/64 pixel (P9).
        float advance = 0.0f;

        /// Pen to the left edge of the glyph's picture. May be negative (ink left of the pen).
        std::int16_t bearing_x = 0;

        /// Top of the line cell to the top edge of the glyph's picture (the cell top is
        /// `baseline` above the pen). Ink above the cell gives a negative offset.
        std::int16_t y_offset = 0;
    };

    /**
     * @brief Metrics of the whole font (font_plan.md F6).
     *
     * Given to the builder as they are; the builder does not derive any of them.
     */
    struct NEUTRINO_EXPORT font_properties {
        /// Distance from one line's top to the next line's top.
        std::int16_t line_height = 0;

        /// Distance from the top of the line cell down to the baseline the text sits on.
        std::int16_t baseline = 0;

        /// Advance of a space, for fonts without a space glyph.
        std::int16_t space_advance = 0;

        /// Extra gap the format adds after every glyph, on top of its advance.
        std::int16_t letter_spacing = 0;

        /// Glyph drawn for code points the font does not map.
        std::uint32_t default_glyph = 0;
    };

    enum class glyph_atlas_format {
        alpha,
        indexed,
        rgba
    };

    template<glyph_atlas_format format>
    class glyph_atlas_builder;

    class NEUTRINO_EXPORT glyph_atlas {
        friend struct details::glyph_atlas_access;

        template<glyph_atlas_format format>
        friend class glyph_atlas_builder;

        public:
            [[nodiscard]] std::size_t glyph_count() const noexcept;
            /// @throws std::runtime_error when @p idx is not less than @ref glyph_count.
            [[nodiscard]] const glyph_properties& get_glyph_properties(std::size_t idx) const;

            [[nodiscard]] font_properties get_font_properties() const noexcept;

        private:
            glyph_atlas(const font_properties& font_props, image_atlas&& atlas, std::vector <glyph_properties>&& props);

        private:
            font_properties m_font_props;
            image_atlas m_atlas;
            std::vector <glyph_properties> m_props;
    };

    namespace details {
        template<glyph_atlas_format format>
        struct glyph_canvas_pixel;

        template<>
        struct glyph_canvas_pixel <glyph_atlas_format::alpha> {
            using type = std::uint8_t;
            static constexpr auto kind = image_kind::rgba;
        };

        template<>
        struct glyph_canvas_pixel <glyph_atlas_format::indexed> {
            using type = std::uint8_t;
        };

        template<>
        struct glyph_canvas_pixel <glyph_atlas_format::rgba> {
            using type = color;
            static constexpr auto kind = image_kind::rgba;
        };

        template<glyph_atlas_format format>
        class glyph_canvas_base {
            public:
                using pixel_type = typename glyph_canvas_pixel <format>::type;

            public:
                glyph_canvas_base()
                    : m_min_x(std::numeric_limits <int>::max()),
                      m_min_y(std::numeric_limits <int>::max()),
                      m_max_x(std::numeric_limits <int>::min()),
                      m_max_y(std::numeric_limits <int>::min()) {
                }

                /// One pixel, relative to the pen on the baseline (x right, y down).
                void put(int x, int y, pixel_type pixel) {
                    fill_run(x, y, 1, pixel);
                }

                /// A run of pixels: @p pixels[i] goes to (@p x + i, @p y). Every pixel counts,
                /// including coverage 0 (P6). Kept as one run, copied whole into the picture.
                void put_span(int x, int y, std::span <const pixel_type> pixels) {
                    if (pixels.empty()) {
                        return;
                    }
                    const int n = static_cast <int>(pixels.size());
                    grow(x, x + n - 1, y);
                    m_commands.push_back({x, y, n, pixel_type{}, static_cast <std::uint32_t>(m_pool.size())});
                    m_pool.insert(m_pool.end(), pixels.begin(), pixels.end());
                }

                /// @p mask with its top-left at (@p x, @p y): each solid pixel puts @p on, the
                /// others write nothing. Each row's solid runs are kept as fills.
                void put_mask(int x, int y, const bitmask& mask, pixel_type on) {
                    for (int my = 0; my < mask.height(); my++) {
                        const auto row = mask.row(my);
                        const int w = mask.width();
                        int i = 0;
                        while (i < w) {
                            while (i < w && !row[static_cast <std::size_t>(i)]) {
                                i++;
                            }
                            const int run = i;
                            while (i < w && row[static_cast <std::size_t>(i)]) {
                                i++;
                            }
                            if (i > run) {
                                fill_run(x + run, y + my, i - run, on);
                            }
                        }
                    }
                }

            protected:
                void clear() {
                    m_min_x = std::numeric_limits <int>::max();
                    m_max_x = std::numeric_limits <int>::min();
                    m_min_y = std::numeric_limits <int>::max();
                    m_max_y = std::numeric_limits <int>::min();
                    m_commands.clear();
                    m_pool.clear();
                }

                [[nodiscard]] dim get_dims() const noexcept {
                    const int w = this->m_max_x - this->m_min_x + 1;
                    const int h = this->m_max_y - this->m_min_y + 1;
                    return {w, h};
                }

                // Replays the writes in order (last write wins, P6) through row spans: the
                // bounds are known, so no per-pixel check.
                template<typename Builder>
                void apply(Builder& builder) {
                    const int h = m_max_y - m_min_y + 1;
                    m_rows.clear();
                    for (int y = 0; y < h; y++) {
                        m_rows.push_back(builder.row(y).data());
                    }
                    using out_pixel = typename Builder::pixel_type;
                    for (const auto& c : m_commands) {
                        auto* dst = static_cast <out_pixel*>(m_rows[static_cast <std::size_t>(c.y - m_min_y)])
                                    + (c.x - m_min_x);
                        if (c.pool == no_pool) {
                            std::fill_n(dst, c.count, to_picture(c.value));
                        } else if constexpr (format == glyph_atlas_format::alpha) {
                            std::transform(m_pool.begin() + c.pool, m_pool.begin() + c.pool + c.count, dst,
                                           to_picture);
                        } else {
                            std::copy_n(m_pool.begin() + c.pool, c.count, dst);
                        }
                    }
                }

                [[nodiscard]] bool empty() const noexcept {
                    return m_commands.empty();
                }

            private:
                static auto to_picture(pixel_type p) noexcept {
                    if constexpr (format == glyph_atlas_format::alpha) {
                        return color{0xFF, 0xFF, 0xFF, p};
                    } else {
                        return p;
                    }
                }

                void fill_run(int x, int y, int count, pixel_type value) {
                    grow(x, x + count - 1, y);
                    m_commands.push_back({x, y, count, value, no_pool});
                }

                void grow(int x0, int x1, int y) noexcept {
                    m_min_x = std::min(x0, m_min_x);
                    m_max_x = std::max(x1, m_max_x);
                    m_min_y = std::min(y, m_min_y);
                    m_max_y = std::max(y, m_max_y);
                }

            protected:
                static constexpr std::uint32_t no_pool = std::numeric_limits <std::uint32_t>::max();

                // A horizontal run of `count` pixels from (x, y): all `value` (a fill, pool ==
                // no_pool), or m_pool[pool, pool + count) (a span).
                struct command {
                    int x;
                    int y;
                    int count;
                    pixel_type value;
                    std::uint32_t pool;
                };

                // Pixels of the spans written to the current glyph.
                std::vector <pixel_type> m_pool;

                // Row starts of the picture being replayed, reused across glyphs.
                std::vector <void*> m_rows;
                int m_min_x;
                int m_min_y;
                int m_max_x;
                int m_max_y;
                std::vector <command> m_commands;
        };

        template<glyph_atlas_format format>
        class glyph_canvas : public glyph_canvas_base <format> {
            protected:
                image create() {
                    image_builder <glyph_canvas_pixel <format>::kind> builder(this->get_dims());
                    if constexpr (format == glyph_atlas_format::alpha) {
                        builder.fill({0xFF, 0xFF, 0xFF, 0});
                    }
                    this->apply(builder);
                    return std::move(builder).build();
                }
        };

        template<>
        class glyph_canvas <glyph_atlas_format::indexed> : public glyph_canvas_base <glyph_atlas_format::indexed> {
            public:
                explicit glyph_canvas(std::optional <std::uint8_t> transparent_index = {})
                    : m_transparent_index(transparent_index),
                      m_palette(nullptr) {
                }

                explicit glyph_canvas(const palette& pal, std::optional <std::uint8_t> transparent_index = {})
                    : m_transparent_index(transparent_index),
                      m_palette(&pal) {
                }

            protected:
                image create() {
                    if (m_palette) {
                        image_builder <image_kind::indexed> builder(this->get_dims(), *m_palette, m_transparent_index);
                        return finish(std::move(builder));
                    }
                    image_builder <image_kind::indexed> builder(this->get_dims(), m_transparent_index);
                    return finish(std::move(builder));
                }

            private:
                // Pixels not written are transparent: the key index when there is one
                // (a fresh image is index 0, which may be a visible colour).
                image finish(image_builder <image_kind::indexed>&& builder) {
                    if (m_transparent_index) {
                        builder.fill(*m_transparent_index);
                    }
                    this->apply(builder);
                    return std::move(builder).build();
                }

            private:
                std::optional <std::uint8_t> m_transparent_index;
                const palette* m_palette;
        };
    }

    template<glyph_atlas_format format>
    class glyph_atlas_builder : public details::glyph_canvas <format> {
        public:
            explicit glyph_atlas_builder(font_properties props)
                : m_font_props(props) {
            }

            glyph_atlas_builder(font_properties props, std::optional <std::uint8_t> transparent_index)
                requires (format == glyph_atlas_format::indexed)
                : details::glyph_canvas <glyph_atlas_format::indexed>(transparent_index),
                  m_font_props(props) {
            }

            glyph_atlas_builder(font_properties props, const palette& pal,
                                std::optional <std::uint8_t> transparent_index)
                requires (format == glyph_atlas_format::indexed)
                : details::glyph_canvas <glyph_atlas_format::indexed>(pal, transparent_index),
                  m_font_props(props) {
            }

            /**
             * @brief Start glyph @p glyph_id. Pixels written until @ref end_glyph belong to it.
             *
             * @param glyph_id The game's id for the glyph; also its picture's bank entry id.
             * @param props The glyph's @c advance; @c glyph, @c bearing_x and @c y_offset are
             *        set by the builder.
             * @throws std::runtime_error when a glyph is already started, or the advance is
             *         not finite.
             */
            void start_glyph(std::uint32_t glyph_id, glyph_properties props = {}) {
                ENFORCE(!m_in_glyph, "start_glyph(", glyph_id, "): the previous glyph is not ended");
                ENFORCE(std::isfinite(props.advance), "start_glyph(", glyph_id, "): the advance is not finite");
                this->clear();
                props.glyph = glyph_id;
                props.advance = std::round(props.advance * 64.0f) / 64.0f; // 26.6 (P9)
                m_props.push_back(props);
                m_in_glyph = true;
            }

            /**
             * @brief Finish the started glyph: crop its pixels to their bounds and derive
             *        @c bearing_x and @c y_offset. A glyph without pixels gets an empty picture.
             *
             * @throws std::runtime_error when no glyph is started, or the glyph lies further than
             *         an @c int16_t from the pen.
             */
            void end_glyph() {
                ENFORCE(m_in_glyph, "end_glyph() without start_glyph()");
                m_in_glyph = false;
                auto& p = m_props.back();
                if (this->empty()) {
                    m_bank.entries.push_back({image{}, p.glyph}); // blank: region of size 0
                    return;
                }
                p.bearing_x = to_int16(this->m_min_x, p.glyph);
                // Pixels are relative to the pen on the baseline; the cell top is baseline above it.
                p.y_offset = to_int16(this->m_min_y + m_font_props.baseline, p.glyph);
                m_bank.entries.push_back({this->create(), p.glyph});
            }

            /// @brief A glyph without pixels (e.g. space): only advances the pen.
            void empty_glyph(std::uint32_t glyph_id, glyph_properties props = {}) {
                start_glyph(glyph_id, props);
                end_glyph();
            }

            /**
             * @brief Pack the glyphs into an atlas. Glyph @c i is picture @c i.
             *
             * @throws std::runtime_error when a glyph is started but not ended, or a glyph id
             *         is used twice.
             */
            glyph_atlas build(std::optional <pixel_format> atlas_texture_format = {},
                              std::optional <dim> max_page = {}) && {
                ENFORCE(!m_in_glyph, "build(): the last glyph is not ended");
                std::unordered_set <std::uint32_t> ids;
                ids.reserve(m_props.size());
                for (const auto& p : m_props) {
                    ENFORCE(ids.insert(p.glyph).second, "build(): glyph", p.glyph, "is defined twice");
                }
                return {
                    m_font_props,
                    build_atlas(m_bank, atlas_texture_format, max_page),
                    std::move(m_props)
                };
            }

        private:
            static std::int16_t to_int16(int v, std::uint32_t glyph) {
                ENFORCE(v >= std::numeric_limits <std::int16_t>::min() && v <= std::numeric_limits <std::int16_t>::max(),
                        "Glyph", glyph, "is too far from the pen:", v);
                return static_cast <std::int16_t>(v);
            }

            font_properties m_font_props;
            image_bank m_bank;
            std::vector <glyph_properties> m_props;
            bool m_in_glyph = false;
    };
}
