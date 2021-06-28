//
// Created by igor on 21/06/2021.
//
#include <cstring>
#include <neutrino/engine/systems/video/oldschool.hh>

namespace neutrino::engine::systems::video {
    oldschool::oldschool(int width, int height)
    : video_system<retro::renderer>(width, height, false)
    {

    }
    // -----------------------------------------------------------------------
    void oldschool::palette(const sdl::palette& pal) {
        surface().set_palette(pal);
    }
    // -----------------------------------------------------------------------
    std::tuple<void*, std::size_t, unsigned, unsigned> oldschool::pixels_data () const {
        return surface().pixels_data();
    }
    // -----------------------------------------------------------------------
    uint8_t* oldschool::pixels() {
        void* px;
        std::tie(px, std::ignore, std::ignore, std::ignore) = pixels_data();
        return reinterpret_cast<uint8_t *>(px);
    }
    // -----------------------------------------------------------------------
    void oldschool::cls() {
        void* px;
        unsigned w, h;
        std::tie(px, std::ignore, w, h) = pixels_data();
        std::memset(px, 0, w*h);
    }
}