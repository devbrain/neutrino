//
// Created by igor on 28/10/2026.
//

#include <neutrino/video/palette.hh>
#include <failsafe/enforce.hh>

namespace neutrino {
    palette load_palette(std::span <const std::uint8_t> bytes, palette_encoding enc) {
        ENFORCE(!bytes.empty())("Can not create an empty palette");
        ENFORCE(bytes.size() % 3 == 0)("Number of bytes in the palette should be divisible by 3");
        const std::size_t n = bytes.size() / 3;
        ENFORCE(n <= 256)("Can not create a palette with more than 256 entries, got ")(n);

        auto p = sdlpp::palette::create(static_cast <int>(n));
        ENFORCE(p);

        palette pal = std::move(*p);

        const auto expand = [enc](std::uint8_t v) -> std::uint8_t {
            if (enc == palette_encoding::vga6) {
                ENFORCE(v < 64)("VGA6 palette is corrupted");
                return static_cast <std::uint8_t>((v << 2) | (v >> 4));
            }
            return v;
        };

        for (std::size_t i = 0; i < n; i++) {
            const auto idx = 3 * i;
            auto r = expand(bytes[idx + 0]);
            auto g = expand(bytes[idx + 1]);
            auto b = expand(bytes[idx + 2]);

            ENFORCE(pal.set_color(i, {r, g, b}));
        }
        return pal;
    }
}
