//
// Created by igor on 14/06/2021.
//

#ifndef NEUTRINO_RENDERER_VGA_RENDERER_HH
#define NEUTRINO_RENDERER_VGA_RENDERER_HH

#include <neutrino/engine/windows/basic_window.hh>
#include <neutrino/engine/renderer/basic_renderer.hh>

#include <neutrino/sdl/surface.hh>
#include <neutrino/sdl/palette.hh>

namespace neutrino::engine {
    class vga_renderer : public basic_renderer {
    public:
        ~vga_renderer() override;

        static sdl::palette standard_palette();
    private:
        void open(const basic_window& window) override;
        void clear() override;
        void present() override;
        [[nodiscard]] basic_window::window_kind_t window_kind() const noexcept override;
        void invalidate(const basic_window& window) override;

    protected:
        [[nodiscard]] sdl::surface& surface() noexcept;
        [[nodiscard]] sdl::surface& surface() const noexcept;
    private:
        struct impl;
        impl* m_pimpl;
    };
}


#endif
