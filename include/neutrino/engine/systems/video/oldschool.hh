//
// Created by igor on 21/06/2021.
//

#ifndef NEUTRINO_SYSTEMS_VIDEO_OLDSCHOOL_HH
#define NEUTRINO_SYSTEMS_VIDEO_OLDSCHOOL_HH

#include <tuple>

#include <neutrino/engine/systems/video/video_system.hh>
#include <neutrino/engine/systems/video/retro/retro.hh>
#include <neutrino/sdl/palette.hh>

namespace neutrino::engine::systems::video {
    class oldschool : public video_system<retro::renderer> {
    public:
        oldschool(int width, int height);

        void palette(const sdl::palette& pal);
        // returns pixels, pitch, w, h
        [[nodiscard]] std::tuple<void*, std::size_t, unsigned, unsigned> pixels_data () const;
        [[nodiscard]] uint8_t* pixels();

        void cls();
    };
}

#endif
