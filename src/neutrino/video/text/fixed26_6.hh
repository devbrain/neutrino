//
// Created by igor on 08/11/2026.
//

#pragma once

#include <cmath>
#include <compare>
#include <cstdint>

namespace neutrino::details {
    /**
     * @brief A length in 1/64 pixel: 26.6 fixed point, FreeType's unit (font_plan.md P9).
     *
     * Registered fonts keep glyph advances in it so fractional (TTF) advances add up without
     * drift; layout rounds each glyph's position to a whole pixel. Public glyph_properties
     * state advances as float pixels, converted when a font is registered.
     */
    class fixed26_6 {
        public:
            constexpr fixed26_6() noexcept = default;

            [[nodiscard]] static constexpr fixed26_6 from_raw(std::int32_t sixty_fourths) noexcept {
                fixed26_6 v;
                v.m_raw = sixty_fourths;
                return v;
            }

            [[nodiscard]] static constexpr fixed26_6 from_pixels(std::int32_t pixels) noexcept {
                return from_raw(pixels * 64);
            }

            /// Rounded to the nearest 1/64 pixel.
            [[nodiscard]] static fixed26_6 from_float(float pixels) noexcept {
                return from_raw(static_cast <std::int32_t>(std::lround(pixels * 64.0f)));
            }

            /// The value in 1/64 pixel.
            [[nodiscard]] constexpr std::int32_t raw() const noexcept {
                return m_raw;
            }

            [[nodiscard]] constexpr float to_float() const noexcept {
                return static_cast <float>(m_raw) / 64.0f;
            }

            constexpr bool operator ==(const fixed26_6&) const noexcept = default;
            constexpr auto operator <=>(const fixed26_6&) const noexcept = default;

        private:
            std::int32_t m_raw = 0;
    };

}
