//
// Created by igor on 21/06/2021.
//

#ifndef NEUTRINO_APPLICATION_OLDSCHOOL_DEMO_HH
#define NEUTRINO_APPLICATION_OLDSCHOOL_DEMO_HH

#include <neutrino/engine/application/neutrino_application.hh>
#include <neutrino/engine/systems/video/oldschool.hh>

namespace neutrino::engine {
    class oldschool_demo : public application<oldschool_demo> {
        friend class application<oldschool_demo>;
    public:
        oldschool_demo ();
        oldschool_demo (int w, int h);
    private:
        [[nodiscard]] std::shared_ptr<systems::video::oldschool> video_system() const noexcept;
    private:
        std::shared_ptr<systems::video::oldschool> m_video_system;
    };
}

#endif
