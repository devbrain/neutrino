//
// Created by igor on 06/06/2021.
//

#include <neutrino/sdl/system.hh>
#include <neutrino/engine/application.hh>
#include <neutrino/engine/window.hh>

struct application :
        public neutrino::engine::single_window_application<neutrino::engine::window_2d, application>
{
public:
    void on_event(const neutrino::events::keyboard& ev)
    {
        if (ev.pressed) {
            if (ev.code == neutrino::events::scan_code_t::RETURN && (ev.mod & neutrino::events::key_mod_t::ALT)) {
                toggle_fullscreen();
            }
            if (ev.code == neutrino::events::scan_code_t::ESCAPE) {
                quit();
            }
        }
    }

    void update(std::chrono::milliseconds ms)
    {

    }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    application app;
    app.open(320,200);
    app.run(30);
}