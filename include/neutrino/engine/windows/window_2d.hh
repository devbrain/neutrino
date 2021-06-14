//
// Created by igor on 12/06/2021.
//

#ifndef NEUTRINO_ENGINE_WINDOWS_WINDOW_2D_HH
#define NEUTRINO_ENGINE_WINDOWS_WINDOW_2D_HH

#include <neutrino/engine/windows/engine_window.hh>

namespace neutrino::engine {
    class window_2d : public engine_window {
    public:
        window_2d();
        window_2d(window_flags_t flags);
        ~window_2d() override;
    protected:
        void render() override;
    protected:
        void clear() override;
        void after_window_opened(uint32_t window_id) override;
        void before_window_destroy() override;
        void present() override;
    private:
        struct impl;
        std::unique_ptr <impl> m_pimpl;
    };
}

#endif
