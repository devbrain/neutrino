//
// Created by igor on 24/10/2026.
//

#pragma once

/**
 * @file physics.hh
 * @brief Public physics surface: geometry shapes/queries and the collision world.
 *
 * Only the two client entry points are listed. The broadphase building blocks (grid, AABB tree,
 * sorted array) and collide/world_internal.hh are implementation details that world.hh pulls in
 * itself; they are not supported entry points for game code.
 */

#include <neutrino/physics/geometry/shapes.hh>  // gathers all geometry headers (overlap, sweep, clip, ...)
#include <neutrino/physics/collide/world.hh>    // collision world + world_types.hh + units.hh
