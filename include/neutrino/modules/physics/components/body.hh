//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_MODULES_PHYSICS_COMPONENTS_BODY_COMPONENT_HH
#define NEUTRINO_MODULES_PHYSICS_COMPONENTS_BODY_COMPONENT_HH

#include <neutrino/neutrino_export.hh>
#include <box2d/b2_body.h>

namespace neutrino {
    struct NEUTRINO_EXPORT body {
        b2Vec2 position;
    };
}

#endif
