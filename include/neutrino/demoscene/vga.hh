//
// Created by igor on 03/07/2021.
//

#ifndef NEUTRINO_DEMOSCENE_VGA_HH
#define NEUTRINO_DEMOSCENE_VGA_HH

#include <cstdint>
#include <array>
#include <vector>
#include <neutrino/hal/video/color.hh>
#include <neutrino/hal/video/texture.hh>
#include <neutrino/hal/video/renderer.hh>
#include <neutrino/engine/main_window.hh>

namespace neutrino::demoscene
{
    class scene;
    class main_scene;
    class vga
    {
        friend class scene;
        friend class main_scene;
    public:
        using palette_t = std::array<hal::color, 256>;
        using surface_t = std::vector<uint8_t>;

        palette_t& palette ();
        surface_t& surface();

        [[nodiscard]] int width () const noexcept;
        [[nodiscard]] int height () const noexcept;


    private:
        vga (engine::main_window& window);
        void present();
    private:
        engine::main_window& m_window;

        palette_t m_palette;
        surface_t m_surface;

        hal::renderer m_renderer;
        hal::texture m_texture;
    };
}

#endif //NEUTRINO_VGA_HH
