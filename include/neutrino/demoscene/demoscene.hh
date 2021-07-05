//
// Created by igor on 03/07/2021.
//

#ifndef NEUTRINO_DEMOSCENE_HH
#define NEUTRINO_DEMOSCENE_HH

#include <vector>
#include <array>
#include <neutrino/engine/application.hh>
#include <neutrino/hal/video/color.hh>

namespace neutrino::demoscene
{
    using palette_t = std::array<hal::color, 256>;
    using surface_t = std::vector<uint8_t>;
    class main_scene;

    class scene : public engine::application {
        friend class main_scene;
    public:
        scene(int w, int h);
        ~scene() override;
    protected:
        virtual void init(palette_t& pal, surface_t& surface, int w, int h) = 0;
        virtual void effect(palette_t& pal, surface_t& surface, int w, int h) = 0;
    private:
        void setup() override;
    private:
        main_scene* m_main_scene;
    };
}

#endif
