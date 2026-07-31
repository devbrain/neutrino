//
// See mechanics.hh.
//

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <variant>

#include <neutrino/physics/geometry/shapes.hh>

#include <ke/game/mechanics.hh>
#include <ke/game/model.hh>
#include <ke/game/sfx.hh>
#include <ke/assets/registry.hh>
#include <ke/assets/backdrop.hh>

namespace {
    // Physics entity ids: bricks are keyed by their index in level_info::bricks (0..N-1);
    // the paddle, the walls and the ball take sentinels well above any brick count.
    constexpr neutrino::physics::entity_id_t EID_PADDLE = 1u << 20;
    constexpr neutrino::physics::entity_id_t EID_WALL = (1u << 20) + 1u;
    constexpr neutrino::physics::entity_id_t EID_BALL_BASE = 1u << 21; // ball i -> EID_BALL_BASE + i

    constexpr float ke_ball_speed = 120.0f; // px/s the ball travels (tune)
    constexpr float ke_fling_speed = 160.0f; // px/s a dead brick sails off at (tune)
    constexpr float ke_capsule_fall_speed = 80.0f; // px/s a dropped bonus descends (catchable: < ball)
    constexpr float ke_ball_speed_min = 60.0f; // slow-ball floor: still trackable
    constexpr float ke_ball_speed_max = 240.0f; // fast-ball ceiling: still catchable
    constexpr std::size_t ke_max_balls = 8; // multiball cap so repeated splits can't explode

    neutrino::physics::aabb box(float x0, float y0, float x1, float y1) {
        return neutrino::physics::aabb{neutrino::physics::vec{x0, y0}, neutrino::physics::vec{x1, y1}};
    }

    neutrino::physics::circle circle(float x0, float y0, float x1, float y1) {
        auto cx = (x0 + x1) / 2.0f;
        auto cy = (y0 + y1) / 2.0f;
        auto r = std::max(cx - x0, cy - y0);
        return neutrino::physics::circle{neutrino::physics::vec{cx, cy}, r};
    }

    // Centre of a ball collider whatever its shape. get_shape() returns the wide shape_t, so
    // the read-back must handle both the box and the circle form -- otherwise a shape swap
    // silently freezes ball.pos (get_if<aabb> is null for a circle).
    std::optional <neutrino::world_point> shape_center(const neutrino::physics::shape_t& sh) {
        if (const auto* bb = std::get_if <neutrino::physics::aabb>(&sh)) {
            return neutrino::world_point{
                (bb->min.x() + bb->max.x()) * 0.5f,
                (bb->min.y() + bb->max.y()) * 0.5f
            };
        }
        if (const auto* c = std::get_if <neutrino::physics::circle>(&sh)) {
            return neutrino::world_point{c->center.x(), c->center.y()};
        }
        return std::nullopt;
    }

    // Reflect velocity v about surface normal n:  v - 2(v.n)n.
    neutrino::world_point reflect(neutrino::world_point v, const neutrino::physics::vec& n) {
        const float d = v.x * n.x() + v.y * n.y();
        return {v.x - 2.0f * d * n.x(), v.y - 2.0f * d * n.y()};
    }

    // v rescaled to length `speed` (unchanged if v is zero).
    neutrino::world_point scaled(neutrino::world_point v, float speed) {
        const float len = std::hypot(v.x, v.y);
        return len > 0.0f ? neutrino::world_point{v.x / len * speed, v.y / len * speed} : v;
    }

    // v rotated by `deg` degrees. Used to fan split (multiball) balls apart.
    neutrino::world_point rotate(neutrino::world_point v, float deg) {
        const float r = deg * 3.14159265f / 180.0f;
        const float c = std::cos(r);
        const float s = std::sin(r);
        return {v.x * c - v.y * s, v.x * s + v.y * c};
    }

    // Paddle "english": outgoing direction from where the ball struck relative to the paddle
    // centre (centre -> straight up, edges -> up to 60 deg), preserving the ball's speed.
    neutrino::world_point paddle_bounce(float ball_x, const paddle_info& p, int pw,
                                        neutrino::world_point vel) {
        const float speed = std::max(1.0f, std::hypot(vel.x, vel.y));
        const float half = pw > 0 ? static_cast <float>(pw) * 0.5f : 1.0f;
        const float off = std::clamp((ball_x - (static_cast <float>(p.x) + half)) / half, -1.0f, 1.0f);
        const float ang = off * 1.0472f; // 60 degrees
        return {std::sin(ang) * speed, -std::cos(ang) * speed}; // up is -y (screen space)
    }
}

void game_mechanics::load(model& m) {
    m_world.clear();
    m_brick_colliders.clear();
    m_balls.clear();
    m_paddle = {};

    build_world_bounds(m);
    build_bricks(m);
    build_paddle(m);

    const paddle_info& p = m.get_paddle();
    auto& li = m.get_level_info();
    li.clear();
    // Launch the initial ball just above the paddle, up-and-sideways. (Split/extra-ball bonuses
    // add more later through the same helper.)
    spawn_ball(m,
               {
                   static_cast <float>(p.x) + static_cast <float>(p.w) * 0.5f,
                   static_cast <float>(p.y) - 4.0f
               }, // sits half (2) + 2px clear above the paddle top
               scaled({0.4f, -1.0f}, ke_ball_speed));
}

void game_mechanics::spawn_ball(model& m, neutrino::world_point pos, neutrino::world_point vel) {
    // Add a ball (a bullet) at `pos` moving `vel`. Shared by load() and the multiball bonus.
    // Balls are only ever APPENDED -- a lost ball is deactivated in place, never erased -- so a
    // ball's index in level_info::balls stays equal to eid - EID_BALL_BASE, which handle_balls
    // relies on to map an event's mover back to its ball.
    level_info& li = m.get_level_info();

    ball_state ball;
    ball.kind = rs::ke_ball_kind::ordinary;
    ball.size = 3;
    ball.half = 2;
    ball.pos = pos;
    ball.vel = vel;
    ball.active = true;

    const auto hs = static_cast <float>(ball.half);
    neutrino::physics::bullet body;
    body.shape = circle(pos.x - hs, pos.y - hs, pos.x + hs, pos.y + hs);
    body.velocity = neutrino::physics::vec{vel.x, vel.y};
    // NOTE: deliberately left at the default `stop`, NOT bullet_on_hit::bounce.
    //
    // Same-step bounce is correct in the engine and would remove the ball's visible one-frame rest
    // against a surface, but KE cannot consume it yet: handle_balls drains the event buffer AFTER
    // run() returns and reconstructs the contact from get_shape(m_balls[bi]) -- both to place the
    // spark effect and to derive the paddle-english x. Under `bounce` the shape has already
    // travelled the post-impact remainder, so that read yields the REBOUND position rather than the
    // impact position: sparks land off the contact and english is computed from the wrong offset,
    // worsening with ball speed and step length. Adopting bounce needs the impact point carried on
    // the BULLET_HIT event (world_event has the normal and toi but no position); until then the
    // one-frame rest is the lesser artefact. See roadmap Tier 2.
    body.on_hit = neutrino::physics::bullet_on_hit::stop;

    const auto eid = static_cast <neutrino::physics::entity_id_t>(EID_BALL_BASE + li.balls.size());
    m_balls.push_back(m_world.add(eid, body));
    li.balls.push_back(ball);
}

void game_mechanics::build_world_bounds(const model& m) {
    const auto [left, right, top, bottom] = m.get_bounds();
    m_bottom_margin = bottom;

    // Walls the ball bounces off: left/right pillars and the top bar. The bottom is open --
    // a ball that falls past it is lost (detected by position in tick()).
    const auto L = static_cast <float>(left);
    const auto R = static_cast <float>(right);
    const auto T = static_cast <float>(top);
    const auto B = static_cast <float>(bottom);
    for (const neutrino::physics::aabb& wall : {
             box(0.0f, T, L, B), // left pillar
             box(R, T, static_cast <float>(rs::ke_screen_w), B), // right pillar
             box(L, 0.0f, R, T)
         }) {
        // top bar
        neutrino::physics::static_body body;
        body.shape = wall;
        m_world.add(EID_WALL, body);
    }
}

void game_mechanics::build_bricks(const model& m) {
    const auto& assets = rs::require_ke_assets();
    // Bricks: one static body per domain brick; eid = index; handle kept parallel so the
    // brick can be removed on death without the model knowing collider ids exist.
    const level_info& li = m.get_level_info();
    m_brick_colliders.resize(li.bricks.size());
    for (std::size_t i = 0; i < li.bricks.size(); ++i) {
        const brick& b = li.bricks[i];
        neutrino::rect tr{};
        if (b.frame >= 0) {
            if (const auto r = assets.bricks.frame_rect(static_cast <std::size_t>(b.frame))) {
                tr = *r;
            }
        }
        neutrino::physics::static_body body;
        body.shape = box(b.pos.x, b.pos.y, b.pos.x + static_cast <float>(tr.w),
                         b.pos.y + static_cast <float>(tr.h));
        m_brick_colliders[i] = m_world.add(static_cast <neutrino::physics::entity_id_t>(i), body);
    }
}

void game_mechanics::build_paddle(const model& m) {
    // Paddle: a kinematic body sized to its current form, re-synced each frame in tick().
    const paddle_info& p = m.get_paddle();
    neutrino::physics::kinematic_body body;
    body.shape = box(static_cast <float>(p.x), static_cast <float>(p.y),
                     static_cast <float>(p.x + p.w), static_cast <float>(p.y + p.h));
    m_paddle = m_world.add(EID_PADDLE, body);
}

void game_mechanics::handle_paddle(model& m) {
    // Positional intent (world::set_target): hand the world the paddle's target CENTRE and let
    // move-and-slide clamp it to the wall pillars. There is no (target-current)/dt velocity to
    // fabricate any more -- an out-of-range mouse (its render x runs the full screen, but the paddle
    // only reaches between the pillars) simply stops AT the pillar, its effective velocity reading
    // ~0. That fabricated into-wall velocity WAS the intermittent paddle-freeze; it can no longer be
    // expressed. The collider keeps its load-time size (fixed until a power-up resizes it); the
    // resolved position is read back in run_step after the world steps.
    const paddle_info& p = m.get_paddle();
    const float cx = static_cast <float>(p.target_x); // target_x is the desired paddle centre
    const float cy = static_cast <float>(p.y) + static_cast <float>(p.h) * 0.5f;
    m_world.set_target(m_paddle, neutrino::physics::vec{cx, cy});
}

void game_mechanics::handle_balls(model& m, const neutrino::physics::world_event& e) {
    level_info& li = m.get_level_info();
    const auto& assets = rs::require_ke_assets();

    auto mover = m_world.get_eid(e.mover);;
    const std::size_t bi = mover - EID_BALL_BASE;
    if (bi >= li.balls.size() || !li.balls[bi].active) {
        return;
    }
    ball_state& ball = li.balls[bi];
    auto spawn_hit = [&](neutrino::sprite_animation_id anim, const rs::ke_anim& src) {
        if (const auto c = shape_center(m_world.get_shape(m_balls[bi]))) {
            const auto r = static_cast <float>(ball.half);
            const neutrino::rect fr = assets.balls.require_frame_rect(src.frames[0]);
            const neutrino::world_point pos{
                c->x - e.normal.x() * r - static_cast <float>(fr.w) * 0.5f,
                c->y - e.normal.y() * r - static_cast <float>(fr.h) * 0.5f
            };
            li.effects.push_back({pos, neutrino::create_sprite_state(anim)});
        }
    };

    auto spawn_capsule = [&](const brick& b) {
        // (optional, authentic) only one capsule may fall at a time:
        // if (std::any_of(li.capsules.begin(), li.capsules.end(),
        //                 [](const capsule& c) { return c.active; })) return;

        const rs::ke_anim& anim = rs::bonus_capsule(b.bonus); // frames live in KE_SPELL (assets.balls)
        const neutrino::rect fr = assets.balls.require_frame_rect(anim.frames[0]);

        capsule cap;
        cap.bonus = b.bonus;
        cap.mag = b.bonus_mag;
        // Centre the capsule sprite on the brick's cell footprint (KE_SPELL pivots top-left, like the
        // hit effects, so pos is the top-left the batch draws at). Same ke_cell_w/h as model.cc.
        cap.pos = {
            b.pos.x + (static_cast <float>(rs::ke_cell_w) - static_cast <float>(fr.w)) * 0.5f,
            b.pos.y + (static_cast <float>(rs::ke_cell_h) - static_cast <float>(fr.h)) * 0.5f,
        };
        cap.w = fr.w;
        cap.h = fr.h;
        cap.active = true;
        cap.state = neutrino::create_sprite_state(
            assets.capsule_anim_id[static_cast <std::size_t>(b.bonus)]); // looping falling anim

        li.capsules.push_back(cap);
    };

    // The target may already be GONE: with multiball, two balls can hit the same brick during one
    // world::run, and handling the first event removed that brick's collider -- so this event's
    // handle is stale and get_eid would abort (taking the whole gameplay scene down with it). The
    // bounce still physically happened against the brick during the sweep, so reflect the ball;
    // just do not damage, re-score, or re-sound a brick that is already dead.
    if (!m_world.is_valid(e.target)) {
        ball.vel = reflect(ball.vel, e.normal);
        m_world.set_velocity(m_balls[bi], neutrino::physics::vec{ball.vel.x, ball.vel.y});
        return;
    }

    const auto tgt = m_world.get_eid(e.target);
    if (tgt == EID_PADDLE) {
        float ball_x = ball.pos.x;
        if (const auto c = shape_center(m_world.get_shape(m_balls[bi]))) {
            ball_x = c->x;
        }
        paddle_info& p = m.get_paddle();
        ball.vel = paddle_bounce(ball_x, p, p.w, ball.vel);
        ke::audio::instance().play(rs::ke_sfx::bounce_racket);
    } else {
        ball.vel = reflect(ball.vel, e.normal); // wall or brick
        if (tgt < li.bricks.size()) {
            spawn_hit(assets.hit_brick_anim_id, rs::hit_brick_anim);
            brick& b = li.bricks[tgt];
            if (b.hits == -1) {
                ke::audio::instance().play(rs::ke_sfx::brick_metal); // indestructible
            } else if (b.m == brick::motion::ALIVE && --b.hits <= 0) {
                // Guard the anim-table bound, not just `none`: types 28-31 (attr>>2 masked to
                // 0x1F) occur in the level data but have no capsule entry -- spawning one would
                // index past ke_spell_capsule_anim / capsule_anim_id (both 28 wide).
                if (b.bonus != rs::bonus::none
                    && static_cast <std::size_t>(b.bonus) < rs::ke_spell_capsule_anim.size()) {
                    spawn_capsule(b);
                }
                b.m = brick::motion::FLUNG;
                b.vel = scaled(ball.vel, ke_fling_speed); // fly off the way the ball went
                m_world.remove(m_brick_colliders[tgt]);
                m_brick_colliders[tgt] = {};
                ke::audio::instance().play(rs::ke_sfx::brick_break);
            } else {
                // Multi-hit brick survived: drop one durability tier -- the tile id (and so
                // the KE_BRICK block index b.frame) subtracts 0x10, showing the damaged
                // graphic (tab.md §5). Guarded so a low tier can't underflow the frame.
                if (b.frame >= 0x10) {
                    b.frame -= 0x10;
                }
                ke::audio::instance().play(rs::ke_sfx::brick_slide);
            }
        } else {
            spawn_hit(assets.hit_wall_anim_id, rs::hit_wall_anim);
            ke::audio::instance().play(rs::ke_sfx::bounce_wall); // EID_WALL
        }
    }
    m_world.set_velocity(m_balls[bi], neutrino::physics::vec{ball.vel.x, ball.vel.y});
}

void game_mechanics::apply_bonus(model& m, rs::bonus b, int mag) {
    LOG_INFO("Bonus:", b);
    level_info& li = m.get_level_info();
    auto& sfx = ke::audio::instance();
    const int strength = mag > 0 ? mag : 1; // bonus_mag is 1..4; guard a stray 0

    // Rescale every active ball's speed by `factor`, clamped so it stays trackable/catchable.
    // reflect()/paddle_bounce() preserve speed magnitude, so the new speed persists across bounces.
    auto scale_balls = [&](float factor) {
        for (std::size_t i = 0; i < li.balls.size(); ++i) {
            ball_state& ball = li.balls[i];
            if (!ball.active) {
                continue;
            }
            const float cur = std::hypot(ball.vel.x, ball.vel.y);
            const float next = std::clamp(cur * factor, ke_ball_speed_min, ke_ball_speed_max);
            ball.vel = scaled(ball.vel, next);
            m_world.set_velocity(m_balls[i], neutrino::physics::vec{ball.vel.x, ball.vel.y});
        }
    };

    // Split each currently-active ball into two extra copies fanned +/- a spread, capped at
    // ke_max_balls. Seeds are snapshotted first: spawn_ball appends to li.balls (invalidating any
    // in-flight reference), and only the pre-existing balls should split (not the new ones).
    auto split_balls = [&]() {
        struct seed {
            neutrino::world_point pos, vel;
        };
        std::vector <seed> seeds;
        std::size_t live = 0;
        for (const ball_state& ball : li.balls) {
            if (ball.active) {
                seeds.push_back({ball.pos, ball.vel});
                ++live;
            }
        }
        for (const seed& s : seeds) {
            for (const float deg : {20.0f, -20.0f}) {
                if (live >= ke_max_balls) {
                    return;
                }
                spawn_ball(m, s.pos, rotate(s.vel, deg));
                ++live;
            }
        }
    };

    // Grow/shrink the paddle by `delta` size-steps: update the model form AND the collider,
    // keeping the centre fixed and clamping the new box inside the pillars. (Assumes higher size
    // == wider form, KE_RACK's convention; flip the sign if a level shows it reversed.)
    auto resize_paddle = [&](int delta) {
        paddle_info& p = m.get_paddle();
        const rs::ke_paddle_frame_range range = rs::ke_paddle_range(p.state);
        const int new_size = std::clamp(p.size + delta, 1, static_cast <int>(range.count));
        if (new_size == p.size) {
            return; // already at the limit -- nothing to resize
        }
        const float cx = static_cast <float>(p.x) + static_cast <float>(p.w) * 0.5f; // resolved centre
        m.set_paddle_size(new_size); // updates p.w / p.h from the new form's frame
        const playfield_bounds bounds = m.get_bounds();
        const int nx = std::clamp(static_cast <int>(std::lround(cx - static_cast <float>(p.w) * 0.5f)),
                                  bounds.left, bounds.right - p.w);
        p.x = nx;
        m_world.set_shape(m_paddle, box(static_cast <float>(nx), static_cast <float>(p.y),
                                        static_cast <float>(nx + p.w), static_cast <float>(p.y + p.h)));
    };

    switch (b) {
        case rs::bonus::slow_ball:
        case rs::bonus::slow_all_balls:
            scale_balls(1.0f - 0.12f * static_cast <float>(strength));
            sfx.play(rs::ke_sfx::bonus_good);
            break;
        case rs::bonus::fast_ball:
        case rs::bonus::speed_up_all_balls:
            scale_balls(1.0f + 0.12f * static_cast <float>(strength));
            sfx.play(rs::ke_sfx::bonus_good);
            break;
        case rs::bonus::extra_ball:
            split_balls();
            sfx.play(rs::ke_sfx::bonus_create);
            break;
        case rs::bonus::enlarge_paddle:
            resize_paddle(+strength);
            sfx.play(rs::ke_sfx::bonus_plus);
            break;
        case rs::bonus::shrink_paddle:
            resize_paddle(-strength);
            sfx.play(rs::ke_sfx::bonus_minus);
            break;
        case rs::bonus::extra_life:
            m.add_life(strength);
            sfx.play(rs::ke_sfx::bonus_good);
            break;
        case rs::bonus::score_multiplier:
            m.add_score(1000L * strength);
            sfx.play(rs::ke_sfx::bonus_good);
            break;
        default:
            // Not yet implemented (guns/laser, transforms, warp/exit, catch/through ball, area
            // explosion, clear effects/enemies, random, ...): acknowledge the catch so the
            // pipeline stays complete, then no-op.
            sfx.play(rs::ke_sfx::bonus_good);
            break;
    }
}

void game_mechanics::tick(model& m, float dt) {
    level_info& li = m.get_level_info();
    paddle_info& p = m.get_paddle();

    handle_paddle(m);

    // 2. Always step the world: move-and-slide (inside run) is what actually applies the paddle's
    //    velocity, so the paddle must keep reacting even with no ball in play (game over is future
    //    work). The ball handling below already no-ops on inactive/removed balls. Each ball is a
    //    bullet, so e.mover identifies which ball hit (one hit per ball per frame at most).
    static const neutrino::physics::aabb region =
        box(0.0f, 0.0f, static_cast <float>(rs::ke_screen_w), static_cast <float>(rs::ke_screen_h));
    const auto& events = m_world.run(region, dt);

    // The paddle move-and-slid this step (a wall may have stopped it short); read its resolved
    // position back before handling ball hits -- the ball swept against it at that position.
    if (const auto sh = m_world.get_shape(m_paddle);
        const auto* bb = std::get_if <neutrino::physics::aabb>(&sh)) {
        p.x = static_cast <int>(bb->min.x());
        p.y = static_cast <int>(bb->min.y());
    }

    for (const neutrino::physics::world_event& e : events) {
        if (e.kind != neutrino::physics::event_kind::BULLET_HIT) {
            continue;
        }
        const auto mover = m_world.get_eid(e.mover);
        if (mover < EID_BALL_BASE) {
            continue;
        }
        handle_balls(m, e);
    }

    // 3. Read each ball's position back (for drawing) and drop balls that fell out.
    for (std::size_t i = 0; i < li.balls.size(); ++i) {
        ball_state& ball = li.balls[i];
        if (!ball.active) {
            continue;
        }
        if (const auto c = shape_center(m_world.get_shape(m_balls[i]))) {
            ball.pos = *c;
        }
        if (ball.pos.y - static_cast <float>(ball.half) > static_cast <float>(m_bottom_margin)) {
            ball.active = false; // fell past the paddle -> lost
            m_world.remove(m_balls[i]);
            m_balls[i] = {};
        }
    }

    // 4. Slide any flung bricks (off-screen culling comes with the dynamic-brick rendering).
    for (brick& b : li.bricks) {
        if (b.m == brick::motion::FLUNG) {
            b.pos.x += b.vel.x * dt;
            b.pos.y += b.vel.y * dt;
        }
    }

    // 5. Slide bonus capsules
    for (auto& cap : li.capsules) {
        cap.pos.y += ke_capsule_fall_speed * dt;
        if (neutrino::physics::intersects(cap.box(), p.box())) {
            apply_bonus(m, cap.bonus, cap.mag);
            cap.active = false;
        } else if (cap.pos.y > m_bottom_margin) {
            cap.active = false; // missed
        }
    }

    std::erase_if(li.effects, [](const hit_effect& fx) {
        if (neutrino::sprite_state_finished(fx.state)) {
            neutrino::unregister_sprite_state(fx.state); // <-- don't leak the playhead
            return true;
        }
        return false;
    });
    std::erase_if(li.capsules, [](const capsule& c) {
        if (!c.active) {
            neutrino::unregister_sprite_state(c.state);
            return true;
        }
        return false;
    });
}
