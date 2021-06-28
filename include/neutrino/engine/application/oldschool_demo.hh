//
// Created by igor on 21/06/2021.
//

#ifndef NEUTRINO_APPLICATION_OLDSCHOOL_DEMO_HH
#define NEUTRINO_APPLICATION_OLDSCHOOL_DEMO_HH

#include <neutrino/engine/application/neutrino_application.hh>
#include <neutrino/engine/systems/video/oldschool.hh>
#include <neutrino/sdl/palette.hh>

namespace neutrino::engine {
    class oldschool_demo : public application<oldschool_demo> {
        friend class application<oldschool_demo>;
    public:
        oldschool_demo ();
        oldschool_demo (int w, int h);
    protected:
        virtual void update(uint8_t* pixels, std::chrono::milliseconds ms) = 0;
        virtual void init_palette(sdl::palette& colors);
        virtual void pre_run();
        [[nodiscard]] std::shared_ptr<systems::video::oldschool> video_system() const noexcept;
    private:
        void after_window_opened() override;
        void update(std::chrono::milliseconds ms) override;
    private:
        std::shared_ptr<systems::video::oldschool> m_video_system;
        sdl::palette                               m_colors;
    };
}

#endif
