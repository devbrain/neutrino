//
// Created by igor on 19/07/2021.
//

#ifndef NEUTRINO_TILED_VIDEO_HH
#define NEUTRINO_TILED_VIDEO_HH

namespace neutrino {
  namespace hal {
    class renderer;
  }

  namespace tiled {
    class world_renderer {
      public:
        explicit world_renderer (hal::renderer& renderer);
      private:
        hal::renderer& m_renderer;
    };
  }
}
#endif
