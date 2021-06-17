//
// Created by igor on 14/06/2021.
//

#ifndef NEUTRINO_RENDERER_OPENGL_RENDERER_HH
#define NEUTRINO_RENDERER_OPENGL_RENDERER_HH

#include <memory>
#include <neutrino/engine/windows/basic_window.hh>
#include <neutrino/engine/renderer/basic_renderer.hh>

namespace neutrino::engine {
    class opengl_renderer : public basic_renderer {
    public:
        ~opengl_renderer() override;
    private:
        void open(const basic_window& window) override;
        void clear() override;
        void present() override;
        basic_window::window_kind_t window_kind() const noexcept override;
    private:
        struct impl;
        std::unique_ptr<impl> m_pimpl;
    };
}


#endif
