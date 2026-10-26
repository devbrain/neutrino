// Exercise SDL's generated main and exit through the update callback.
#include <neutrino/simplex/entry_point.hh>

namespace {
    class smoke_app final : public neutrino::simplex::application {
        public:
            smoke_app() : application(64, 48, "Simplex startup smoke test") {}
            bool on_update(float, const neutrino::input_snapshot&) override { return false; }
    };
}

SIMPLEX_MAIN(smoke_app)
