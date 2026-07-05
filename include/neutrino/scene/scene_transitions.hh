/// @file scene_transitions.hh
/// @brief Global scene-stack transition functions.
///
/// The application is effectively a singleton, so scene transitions are
/// exposed as free functions. They post a request to the SDL event queue
/// (applied on the next event pump), which makes them safe to call from
/// within scene callbacks and from any point in the frame.

#pragma once

#include <memory>

#include <neutrino/neutrino_export.h>
#include <neutrino/scene/base_scene.hh>

namespace neutrino {
    /// @brief Push a new scene on top of the stack, pausing the current one.
    NEUTRINO_EXPORT void push_scene(std::unique_ptr <base_scene> scene);
    /// @brief Pop the topmost scene, resuming the one below.
    NEUTRINO_EXPORT void pop_scene();
    /// @brief Replace the topmost scene (pop + push as a single operation).
    NEUTRINO_EXPORT void replace_scene(std::unique_ptr <base_scene> scene);
}
