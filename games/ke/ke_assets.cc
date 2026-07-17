//
// See ke_assets.hh. The service accessor holds a non-owning pointer to the scene-owned
// ke_assets, like neutrino's service_locator holds pointers to app-owned services.
//

#include "ke_assets.hh"

#include <failsafe/enforce.hh>

namespace rs {
    namespace {
        ke_assets* g_assets = nullptr;
    }

    void set_ke_assets(ke_assets& assets) {
        g_assets = &assets;
    }

    ke_assets& require_ke_assets() {
        ENFORCE(g_assets != nullptr)("ke_assets has not been published");
        return *g_assets;
    }

    void clear_ke_assets() {
        g_assets = nullptr;
    }
}
