//
// Created by igor on 24/10/2026.
//

#pragma once

/**
 * @file neutrino.hh
 * @brief Umbrella header: the whole public engine surface in one include.
 *
 * A game needs only this header in every translation unit, plus <neutrino/entry_point.hh> in one.
 *
 * Deliberately NOT included:
 *   - <neutrino/entry_point.hh> (NEUTRINO_MAIN) -- it defines the program entry point, so it must
 *     be included by exactly one translation unit; pulling it in here would produce duplicate
 *     symbols in any game with more than one source file.
 *   - <neutrino/simplex/...> -- the simplified application layer; opt in explicitly.
 *   - <neutrino/detail/...> and other internals -- not part of the public surface.
 */

// ---- core --------------------------------------------------------------------------------------
#include <neutrino/application.hh>
#include <neutrino/coordinates.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>

// ---- input -------------------------------------------------------------------------------------
#include <neutrino/input/input.hh>

// ---- video -------------------------------------------------------------------------------------
#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/image_loader.hh>
#include <neutrino/video/texture.hh>
#include <neutrino/video/sprites.hh>
#include <neutrino/video/world.hh>

// ---- world (tile maps) -------------------------------------------------------------------------
#include <neutrino/world/world.hh>
#include <neutrino/world/tmx_loader.hh>

// ---- physics -----------------------------------------------------------------------------------
#include <neutrino/physics/physics.hh>

// ---- audio -------------------------------------------------------------------------------------
#include <neutrino/audio/audio.hh>
