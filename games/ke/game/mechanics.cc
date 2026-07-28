//
// See mechanics.hh.
//

#include <ke/game/mechanics.hh>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <variant>

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
    li.effects.clear();
    // Launch the initial ball: an AABB bullet just above the paddle, up-and-sideways.
    // (More balls -- e.g. a split bonus -- are added later via this same push pattern.)
    li.balls.clear(); {
        ball_state ball;
        ball.kind = rs::ke_ball_kind::ordinary;
        ball.size = 3;
        ball.half = 2;
        ball.pos = {
            static_cast <float>(p.x) + static_cast <float>(p.w) / 2,
            static_cast <float>(p.y - ball.half - 2)
        };
        ball.vel = scaled({0.4f, -1.0f}, ke_ball_speed);
        ball.active = true;

        const auto hs = static_cast <float>(ball.half);
        neutrino::physics::bullet body;
        //body.shape    = box(ball.pos.x - hs, ball.pos.y - hs, ball.pos.x + hs, ball.pos.y + hs);
        body.shape = circle(ball.pos.x - hs, ball.pos.y - hs, ball.pos.x + hs, ball.pos.y + hs);
        body.velocity = neutrino::physics::vec{ball.vel.x, ball.vel.y};

        const auto eid = static_cast <neutrino::physics::entity_id_t>(EID_BALL_BASE + li.balls.size());
        m_balls.push_back(m_world.add(eid, body));
        li.balls.push_back(ball);
    }
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

void game_mechanics::handle_paddle(model& m, float dt) {
    // 1. Drive the paddle toward the player's target via velocity, so the playfield walls
    //    (static bodies) stop it through move-and-slide -- no clamp. The collider keeps its
    //    load-time size; the form is fixed until power-ups resize it.
    auto& p = m.get_paddle();
    const float desired_x = static_cast <float>(p.target_x) - static_cast <float>(p.w) * 0.5f;
    auto cur_x = static_cast <float>(p.x);
    if (const auto sh = m_world.get_shape(m_paddle);
        const auto* bb = std::get_if <neutrino::physics::aabb>(&sh)) {
        cur_x = bb->min.x();
    }
    // The velocity that would reach the target this frame; move-and-slide clips it at a wall.
    const float vx = dt > 0.0f ? (desired_x - cur_x) / dt : 0.0f;
    m_world.set_velocity(m_paddle, neutrino::physics::vec{vx, 0.0f});
}

void game_mechanics::handle_balls(model& m, const neutrino::physics::world_event& e) {
    level_info& li = m.get_level_info();
    auto mover = m_world.get_eid(e.mover);;
    const std::size_t bi = mover - EID_BALL_BASE;
    if (bi >= li.balls.size() || !li.balls[bi].active) {
        return;
    }
    ball_state& ball = li.balls[bi];
    auto spawn_hit = [&](rs::hit_kind k) {
        if (const auto c = shape_center(m_world.get_shape(m_balls[bi]))) {
            const auto r = static_cast <float>(ball.half); // circle radius == ball.half
            li.effects.push_back({{c->x - e.normal.x() * r, c->y - e.normal.y() * r}, k, 0.0f});
        }
    };

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
            spawn_hit(rs::hit_kind::brick);
            brick& b = li.bricks[tgt];
            if (b.hits == -1) {
                ke::audio::instance().play(rs::ke_sfx::brick_metal); // indestructible
            } else if (b.m == brick::motion::ALIVE && --b.hits <= 0) {
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
            spawn_hit(rs::hit_kind::wall);
            ke::audio::instance().play(rs::ke_sfx::bounce_wall); // EID_WALL
        }
    }
    m_world.set_velocity(m_balls[bi], neutrino::physics::vec{ball.vel.x, ball.vel.y});
}

void game_mechanics::tick(model& m, float dt) {
    level_info& li = m.get_level_info();
    paddle_info& p = m.get_paddle();

    handle_paddle(m, dt);

    bool any_active = false;
    for (const ball_state& b : li.balls) {
        any_active = any_active || b.active;
    }
    if (!any_active) {
        return; // all balls lost (game over is future work)
    }

    // 2. Step the world and drain the balls' collision events. Each ball is a bullet, so
    //    e.mover identifies which ball hit (one hit per ball per frame at most).
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

    for (hit_effect& fx : li.effects) {
        fx.elapsed += dt;
    }
    std::erase_if(li.effects, [](const hit_effect& fx) {
        // <vector>, C++20
        const rs::ke_anim& a = rs::hit_anim(fx.kind);
        return fx.elapsed >= a.count * a.ticks * rs::ke_tick_seconds; // one-shot: drop when done
    });
}
