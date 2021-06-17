//
// Created by igor on 06/06/2021.
//

#include <neutrino/engine/application.hh>
#include <neutrino/engine/renderer/vga_renderer.hh>

struct application :
        public neutrino::engine::single_window_application<application>
{
public:
    neutrino::engine::basic_renderer* create_renderer() override {
        return new neutrino::engine::vga_renderer();
    }
    void update([[maybe_unused]] std::chrono::milliseconds ms) override
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