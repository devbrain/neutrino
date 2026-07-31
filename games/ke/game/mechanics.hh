//
// Game mechanics: the rules/simulation layer.
//
// Owns the physical world (a simulation *tool*, not domain state) and steps the game each
// frame, translating physics events into domain changes on the model -- ball bounce/english,
// brick hit -> fling, and (as the game grows) bonus spawns, scoring, life loss. This is the
// only component that touches neutrino::physics; the model stays a pure domain container.
//

#pragma once

#include <vector>

#include <neutrino/physics/collide/world.hh>
#include <neutrino/video/geometry_types.hh> // world_point

#include <ke/resources/cell.hh> // rs::bonus

class model;

class game_mechanics {
    public:
        // Build the physical world from the model's domain (walls, one body per brick, the
        // kinematic paddle) and launch the ball. Call once per level, after model::load_level().
        void load(model& m);

        // Advance one frame: sync the paddle collider to the (input-driven) model paddle, run
        // the ball (a bullet) and drain its collisions into domain changes -- wall/paddle
        // bounces, brick hits + flings -- then slide any flung bricks. @p dt is seconds.
        void tick(model& m, float dt);

    private:
        void build_world_bounds(const model& m);
        void build_bricks(const model& m);
        void build_paddle(const model& m);

        void handle_paddle(model& m);
        void handle_balls(model& m, const neutrino::physics::world_event& e);

        // Add a ball (bullet) at pos moving vel; shared by load() and the extra-ball bonus.
        void spawn_ball(model& m, neutrino::world_point pos, neutrino::world_point vel);
        // Apply a caught capsule's effect (ball speed, multiball, paddle resize, life/score).
        void apply_bonus(model& m, rs::bonus b, int mag);
    private:
        neutrino::physics::world m_world;
        neutrino::physics::collider_id m_paddle{};
        std::vector <neutrino::physics::collider_id> m_balls;           // parallel to model balls
        std::vector <neutrino::physics::collider_id> m_brick_colliders; // parallel to model bricks
        int m_bottom_margin{}; // playfield bottom; a ball past it is lost
};
