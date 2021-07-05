//
// Created by igor on 03/07/2021.
//

#ifndef NEUTRINO_VIDEO_HH
#define NEUTRINO_VIDEO_HH


#include <cstdint>
#include <neutrino/demoscene/demoscene.hh>
#include <neutrino/engine/graphics/accelerated_2d_context.hh>
#include <neutrino/hal/video/color.hh>
#include <neutrino/hal/video/texture.hh>

namespace neutrino::demoscene
{
    class video : public engine::accelerated_2d_context
    {
    public:
        video (int w, int h);
        std::tuple<int, int> dimensions() override;

        palette_t* palette ();
        surface_t* surface();
    private:
        void open(const hal::window& window) override;
        void present() override;

    private:
        int m_width;
        int m_heigh;

        palette_t m_palette;
        surface_t m_surface;

        hal::pixel_format m_format;
        hal::texture m_texture;
    };
}

#endif //NEUTRINO_VIDEO_HH
