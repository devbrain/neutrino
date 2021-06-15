//
// Created by igor on 06/06/2021.
//

#include <neutrino/engine/application.hh>
#include <neutrino/engine/window.hh>

struct application :
        public neutrino::engine::single_window_application<neutrino::engine::window_2d, application>
{
public:

    void update([[maybe_unused]] std::chrono::milliseconds ms)
    {

    }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    application app;
    app.open(320,200);
    app.run(30);
    return 0;
}