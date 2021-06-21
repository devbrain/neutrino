//
// Created by igor on 14/06/2021.
//

#ifndef NEUTRINO_RENDERER_RENDERER_2D_HH
#define NEUTRINO_RENDERER_RENDERER_2D_HH

#include <memory>
#include <neutrino/engine/windows/basic_window.hh>
#include <neutrino/engine/renderer/basic_renderer.hh>

namespace neutrino::engine {
    class renderer_2d : public basic_renderer {
    public:
        ~renderer_2d() override;
    private:
        void open(const basic_window& window) override;
        void clear() override;
        void present() override;
        [[nodiscard]] basic_window::window_kind_t window_kind() const noexcept override;
        void invalidate(const basic_window& window) override;
    private:
        struct impl;
        std::unique_ptr<impl> m_pimpl;
    };
}


#endif
