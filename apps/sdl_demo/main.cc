//
// Created by igor on 06/06/2021.
//

#include <neutrino/engine/application.hh>

struct application :
        public neutrino::engine::oldschool_demo
{
public:

    void update([[maybe_unused]] std::chrono::milliseconds ms) override
    {

    }
};

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
    application app;
    app.open();
    app.run(30);
    return 0;
}