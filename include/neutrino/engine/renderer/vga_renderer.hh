//
// Created by igor on 14/06/2021.
//

#ifndef NEUTRINO_RENDERER_VGA_RENDERER_HH
#define NEUTRINO_RENDERER_VGA_RENDERER_HH

#include <memory>
#include <neutrino/engine/windows/basic_window.hh>
#include <neutrino/engine/renderer/basic_renderer.hh>

namespace neutrino::engine {
    class vga_renderer : public basic_renderer {
    public:
        vga_renderer(const basic_window& window);
        ~vga_renderer() override;
    private:
        void clear() override;
        void present() override;
    private:
        struct impl;
        std::unique_ptr<impl> m_pimpl;
    };
}


#endif
