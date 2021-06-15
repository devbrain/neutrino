//
// Created by igor on 14/06/2021.
//

#ifndef NEUTRINO_RENDERER_BASIC_RENDERER_HH
#define NEUTRINO_RENDERER_BASIC_RENDERER_HH

namespace neutrino::engine {
    class basic_renderer {
    public:
        virtual ~basic_renderer();

        virtual void clear() = 0;
        virtual void present() = 0;

    };
}

#endif
