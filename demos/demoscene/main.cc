#include <memory>
#include <neutrino/application.hh>
#include <sdlpp/app/entry_point.hh>
#include "gallery_scene.hh"

namespace demoscene {

class demoscene_gallery_app final : public neutrino::application {
public:
    demoscene_gallery_app()
        : neutrino::application(make_config()) {}

protected:
    std::unique_ptr<neutrino::base_scene> create_initial_scene() override {
        return std::make_unique<gallery_scene>();
    }

private:
    static neutrino::application_config make_config() {
        neutrino::application_config cfg;
        cfg.title = "Neutrino Demoscene Gallery - The Bas van Gaalen Collection (1994)";
        cfg.width = 640;
        cfg.height = 400; // 320x200 scaled x2
        cfg.logical_size = neutrino::dim{640, 400};
        cfg.scale = neutrino::scale_mode::integer_scale;
        cfg.flags = sdlpp::window_flags::resizable;
        return cfg;
    }
};

} // namespace demoscene

SDLPP_MAIN(demoscene::demoscene_gallery_app)
