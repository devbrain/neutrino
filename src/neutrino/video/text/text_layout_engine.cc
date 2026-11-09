//
// Created by igor on 08/11/2026.
//

#include "video/text/text_layout_engine.hh"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <failsafe/enforce.hh>
#include <onyx_font/text/utf8.hh>

namespace neutrino::details {
    namespace {
        struct decoded_char {
            std::size_t index;
            char32_t cp;
        };

        std::vector <decoded_char> decode(std::string_view text) {
            std::vector <decoded_char> out;
            out.reserve(text.size());
            std::size_t pos = 0;
            while (pos < text.size()) {
                const auto [cp, bytes] = onyx_font::utf8_decode_one(text.substr(pos));
                out.push_back({pos, cp});
                pos += static_cast <std::size_t>(std::max(bytes, 1));
            }
            return out;
        }

        bool breakable(char32_t cp) {
            return cp == U' ' || cp == U'\t';
        }

        // Pens are in 1/64 pixel of the unscaled font (P9); px() scales and rounds to whole
        // pixels (T3: positions snap).
        class metrics {
            public:
                metrics(const loaded_font& font, const text_params& params)
                    : m_font(font),
                      m_scale(params.scale),
                      m_spacing(std::int64_t{font.font_props.letter_spacing + params.letter_spacing} * 64) {
                    ENFORCE(params.scale > 0.0f, "text_params.scale must be positive, got", params.scale);
                    if (const auto it = font.lookup.find(U' '); it != font.lookup.end()) {
                        m_space_glyph = it->second;
                        m_space = font.advances[it->second].raw();
                    } else {
                        m_space = std::int64_t{font.font_props.space_advance} * 64;
                    }
                    m_tab_stop = m_space * std::max(params.tab_width, 1);
                }

                struct advance_result {
                    std::int64_t advance = 0;
                    std::optional <std::size_t> glyph;
                };

                // The character's advance when its origin is at pen (tabs depend on it).
                [[nodiscard]] advance_result advance(char32_t cp, std::int64_t pen) const {
                    switch (cp) {
                        case U'\r':
                        case U'\n':
                            return {};
                        case U'\t':
                            if (m_tab_stop <= 0) {
                                return {};
                            }
                            return {(pen / m_tab_stop + 1) * m_tab_stop - pen, std::nullopt};
                        case U' ':
                            return {m_space, m_space_glyph};
                        default:
                            break;
                    }
                    if (const auto g = m_font.find_glyph(cp)) {
                        return {m_font.advances[*g].raw(), g};
                    }
                    return {};
                }

                // Letter spacing goes between characters, not before a line's first one;
                // '\r' takes no space at all.
                [[nodiscard]] std::int64_t spacing_before(char32_t cp, bool first) const {
                    return first || cp == U'\r' ? 0 : m_spacing;
                }

                [[nodiscard]] int px(std::int64_t sixty_fourths) const {
                    return static_cast <int>(std::lround(static_cast <double>(sixty_fourths) * m_scale / 64.0));
                }

                [[nodiscard]] int scaled(int pixels) const {
                    return static_cast <int>(std::lround(static_cast <double>(pixels) * m_scale));
                }

            private:
                const loaded_font& m_font;
                float m_scale;
                std::int64_t m_spacing;
                std::int64_t m_space = 0;
                std::optional <std::size_t> m_space_glyph;
                std::int64_t m_tab_stop = 0;
        };

        // One line found by the breaking pass, in decoded character indices.
        struct line_spec {
            std::size_t first = 0;       // first character
            std::size_t last = 0;        // one past the last character placed on the line
            std::size_t content_end = 0; // one past the last character that is not a break space
            int width = 0;
            bool newline = false;        // character `last` is the '\n' ending the line
        };

        std::vector <line_spec> break_lines(const std::vector <decoded_char>& chars, const metrics& m,
                                            std::optional <int> wrap_width) {
            std::vector <line_spec> lines;
            const std::size_t n = chars.size();
            std::size_t para = 0;
            while (true) {
                std::size_t para_end = para;
                while (para_end < n && chars[para_end].cp != U'\n') {
                    para_end++;
                }
                std::size_t ls = para;
                while (true) {
                    std::int64_t pen = 0;
                    std::int64_t content_pen = 0;
                    std::size_t content_end = ls;
                    // Break opportunity: the space run after the last word.
                    std::optional <std::size_t> brk_next;
                    std::size_t brk_content_end = ls;
                    std::int64_t brk_pen = 0;
                    bool in_run = false;
                    bool wrapped = false;
                    std::size_t i = ls;
                    while (i < para_end) {
                        const char32_t cp = chars[i].cp;
                        const std::int64_t start = pen + m.spacing_before(cp, i == ls);
                        const auto a = m.advance(cp, start);
                        const std::int64_t next = start + a.advance;
                        if (breakable(cp)) {
                            if (!in_run) {
                                brk_content_end = content_end;
                                brk_pen = content_pen;
                                in_run = true;
                            }
                            pen = next;
                            i++;
                            brk_next = i;
                            continue;
                        }
                        in_run = false;
                        if (wrap_width && i > ls && m.px(next) > *wrap_width) {
                            if (brk_next && brk_content_end > ls) {
                                lines.push_back({ls, *brk_next, brk_content_end, m.px(brk_pen), false});
                                ls = *brk_next;
                            } else {
                                lines.push_back({ls, i, content_end, m.px(content_pen), false});
                                ls = i;
                            }
                            wrapped = true;
                            break;
                        }
                        pen = next;
                        content_pen = next;
                        content_end = i + 1;
                        i++;
                    }
                    if (!wrapped) {
                        lines.push_back({ls, para_end, content_end, m.px(content_pen), para_end < n});
                        break;
                    }
                }
                if (para_end >= n) {
                    break;
                }
                para = para_end + 1; // after the '\n'; text ending in '\n' gets an empty last line
            }
            return lines;
        }

        std::size_t byte_at(const std::vector <decoded_char>& chars, std::size_t i, std::size_t text_size) {
            return i < chars.size() ? chars[i].index : text_size;
        }
    }

    text_layout layout_text(std::string_view text, const loaded_font& font, const text_params& params,
                            std::optional <int> wrap_width) {
        const metrics m(font, params);
        const auto chars = decode(text);
        const auto specs = break_lines(chars, m, wrap_width);

        text_layout out;
        int widest = 0;
        for (const auto& s : specs) {
            widest = std::max(widest, s.width);
        }
        const int box = wrap_width ? *wrap_width : widest;
        out.line_step = m.scaled(font.font_props.line_height + params.line_spacing);

        out.lines.reserve(specs.size());
        for (std::size_t li = 0; li < specs.size(); li++) {
            const auto& s = specs[li];
            laid_out_line line;
            line.range = {byte_at(chars, s.first, text.size()), byte_at(chars, s.content_end, text.size()), s.width};
            if (s.content_end == s.first) {
                line.range.end = line.range.begin;
            }
            switch (params.align) {
                case text_align::left:
                    line.x = 0;
                    break;
                case text_align::center:
                    line.x = std::max(0, (box - s.width) / 2);
                    break;
                case text_align::right:
                    line.x = std::max(0, box - s.width);
                    break;
            }
            line.top = static_cast <int>(li) * out.line_step;

            std::int64_t pen = 0;
            const std::size_t placed_end = s.newline ? s.last + 1 : s.last;
            for (std::size_t i = s.first; i < placed_end; i++) {
                const char32_t cp = chars[i].cp;
                const std::int64_t start = pen + m.spacing_before(cp, i == s.first);
                const auto a = m.advance(cp, start);
                laid_out_char c;
                c.index = chars[i].index;
                c.code_point = cp;
                c.caret_x = line.x + m.px(start);
                c.advance = m.px(start + a.advance) - m.px(start);
                if (a.glyph) {
                    const auto& g = font.glyph_props[*a.glyph];
                    c.glyph = a.glyph;
                    c.picture = {c.caret_x + m.scaled(g.bearing_x), line.top + m.scaled(g.y_offset)};
                }
                line.chars.push_back(c);
                pen = start + a.advance;
            }
            line.end_caret_x = line.x + m.px(pen);
            line.after = byte_at(chars, placed_end, text.size());
            out.lines.push_back(std::move(line));
        }

        const auto n = static_cast <int>(out.lines.size());
        out.size = {widest, (n - 1) * out.line_step + m.scaled(font.font_props.line_height)};
        return out;
    }

    fit_result fit_text(std::string_view text, const loaded_font& font, int width, const text_params& params) {
        const metrics m(font, params);
        fit_result r;
        std::int64_t pen = 0;
        std::size_t pos = 0;
        bool first = true;
        while (pos < text.size()) {
            const auto [cp, bytes] = onyx_font::utf8_decode_one(text.substr(pos));
            if (cp == U'\n') {
                break;
            }
            const std::int64_t start = pen + m.spacing_before(cp, first);
            const std::int64_t next = start + m.advance(cp, start).advance;
            if (m.px(next) > width) {
                break;
            }
            pen = next;
            first = false;
            pos += static_cast <std::size_t>(std::max(bytes, 1));
            r = {pos, m.px(pen)};
        }
        return r;
    }

    point caret_position(const text_layout& layout, std::string_view text, std::size_t index) {
        index = std::min(index, text.size());
        if (index == text.size()) {
            const auto& last = layout.lines.back();
            return {last.end_caret_x, last.top};
        }
        // The character containing index: the last one starting at or before it.
        const laid_out_line* best_line = &layout.lines.front();
        const laid_out_char* best = nullptr;
        for (const auto& line : layout.lines) {
            for (const auto& c : line.chars) {
                if (c.index > index) {
                    return best ? point{best->caret_x, best_line->top} : point{line.x, line.top};
                }
                best = &c;
                best_line = &line;
            }
        }
        return best ? point{best->caret_x, best_line->top} : point{best_line->x, best_line->top};
    }

    std::size_t hit_test(const text_layout& layout, std::string_view text, point position) {
        const auto n = static_cast <int>(layout.lines.size());
        int li = layout.line_step > 0 ? position.y / layout.line_step : 0;
        if (position.y < 0) {
            li = 0;
        }
        li = std::clamp(li, 0, n - 1);
        const auto& line = layout.lines[static_cast <std::size_t>(li)];
        for (const auto& c : line.chars) {
            if (c.code_point == U'\n') {
                return c.index; // past the visible characters: before the line's '\n'
            }
            if (position.x * 2 < c.caret_x * 2 + c.advance) {
                return c.index;
            }
        }
        return std::min(line.after, text.size());
    }
}
