//
// Created by igor on 8/28/24.
//

#ifndef  PROPS_EDITOR_APP_HH
#define  PROPS_EDITOR_APP_HH

#include <neutrino/application.hh>
#include "data_loader/data_directory.hh"

class props_editor_app : public neutrino::application {
    public:
        explicit props_editor_app(const std::filesystem::path& path_to_data);
    private:
        void setup_scenes(neutrino::sdl::renderer& renderer) override;
    private:
        data_directory m_data_dir;
};

#endif
