//
// See mechanics.hh.
//

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include <neutrino/physics/geometry/shapes.hh>

#include <ke/game/mechanics.hh>
#include <ke/game/model.hh>
#include <ke/game/sfx.hh>
#include <ke/assets/registry.hh>
#include <ke/assets/backdrop.hh>

namespace {
    // What a collider belongs to, carried in the physics owner slot (world::set_owner). One
    // value says both WHAT it is and WHICH one, so event routing is a switch on a named kind
    // rather than range comparisons against the entity id.
    //
    // The entity id used to carry this: bricks took their index 0..N-1, the paddle and walls
    // sat at 1<<20, balls at 1<<21 + i, and handling a ball hit meant `mover - EID_BALL_BASE`.
    // Nothing checked those ranges -- a brick index that ever reached 1<<20 would have read as
    // the paddle -- and the subtraction silently required that balls were never erased from the
    // model, only deactivated in place. That constraint still holds (see spawn_ball) but it is
    // no longer load-bearing for identification: the index travels with the collider.
    enum class ke_kind : std::uint32_t { brick, paddle, wall, ball };

    struct ke_owner {
        ke_kind kind{};
        std::uint32_t index{}; // brick / ball index; unused for the paddle and walls
    };
    static_assert(neutrino::physics::collider_owner <ke_owner>);

    // Entity ids are still required by add(); they are no longer how anything is identified, so
    // one shared value per category is enough (the physics only uses eid for its own dedup).
    constexpr neutrino::physics::entity_id_t EID_PADDLE = 1u << 20;
    constexpr neutrino::physics::entity_id_t EID_WALL = (1u << 20) + 1u;
    constexpr neutrino::physics::entity_id_t EID_BALL_BASE = 1u << 21; // ball i -> EID_BALL_BASE + i

    constexpr float ke_ball_speed = 120.0f; // px/s the ball travels (tune)
    constexpr float ke_fling_speed = 160.0f; // px/s a dead brick sails off at (tune)
    constexpr std::size_t ke_max_balls = 25; // original spawn_ball limit (0x2C560)

    // A dropped bonus descends at a fixed rate (catchable: slower than the ball).
    constexpr neutrino::world_velocity ke_capsule_fall{0.0f, 80.0f};

    neutrino::physics::aabb box(float x0, float y0, float x1, float y1) {
        return neutrino::physics::aabb{neutrino::physics::vec{x0, y0}, neutrino::physics::vec{x1, y1}};
    }

    neutrino::physics::circle circle(float x0, float y0, float x1, float y1) {
        auto cx = (x0 + x1) / 2.0f;
        auto cy = (y0 + y1) / 2.0f;
        auto r = std::max(cx - x0, cy - y0);
        return neutrino::physics::circle{neutrino::physics::vec{cx, cy}, r};
    }

    neutrino::physics::aabb enemy_box(const enemy_state& enemy, int inset = 0) {
        const auto r = rs::require_ke_assets().enemies.require_metrics(enemy.frame()).local_bounds();
        // Original 0x21308 uses width/height minus 2; 0x21354 additionally
        // insets by 3 on every side when a ball hits an enemy.
        return box(enemy.pos.x + r.x + inset, enemy.pos.y + r.y + inset,
                   enemy.pos.x + r.x + r.w - 2 - inset,
                   enemy.pos.y + r.y + r.h - 2 - inset);
    }

    // Ball responses. All four take and return a world_velocity: these are RATES, and none of
    // them can now be handed where a position belongs (nor a position handed in). The centre
    // read-back that used to live here as shape_center() is gone -- world::position_of does it,
    // for every shape, and returns a world_pos rather than an anonymous float pair.

    // Reflect velocity v about surface normal n:  v - 2(v.n)n.
    neutrino::world_velocity reflect(neutrino::world_velocity v, const neutrino::physics::vec& n) {
        const float d = v.x * n.x() + v.y * n.y();
        return {v.x - 2.0f * d * n.x(), v.y - 2.0f * d * n.y()};
    }

    // v rescaled to length `speed` (unchanged if v is zero).
    neutrino::world_velocity scaled(neutrino::world_velocity v, float speed) {
        const float len = v.speed();
        return len > 0.0f ? neutrino::world_velocity{v.x / len * speed, v.y / len * speed} : v;
    }

    // Paddle "english": outgoing direction from where the ball struck relative to the paddle
    // centre (centre -> straight up, edges -> up to 60 deg), preserving the ball's speed.
    neutrino::world_velocity paddle_bounce(float ball_x, const paddle_info& p, int pw,
                                           neutrino::world_velocity vel) {
        const float speed = std::max(1.0f, vel.speed());
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
    m_enemy_clock = 0.0;
    m_spawn_index = 0;
    m_spawn_ticks = rs::require_ke_assets().levels[m.get_level()].spawn_period.count();

    build_world_bounds(m);
    build_bricks(m);
    build_paddle(m);

    const paddle_info& p = m.get_paddle();
    auto& li = m.get_level_info();
    li.clear();
    // Launch the initial ball just above the paddle, up-and-sideways. (Extra-ball bonuses
    // add more later through the same helper.)
    spawn_ball(m,
               {
                   static_cast <float>(p.x) + static_cast <float>(p.w) * 0.5f,
                   static_cast <float>(p.y) - 4.0f
               }, // smallest ball's radius (3) + 1px clear above the paddle top
               scaled({0.4f, -1.0f}, ke_ball_speed));
}

void game_mechanics::spawn_ball(model& m, neutrino::world_pos pos, neutrino::world_velocity vel) {
    // Add a ball (a bullet) at `pos` moving `vel`. Shared by load() and the multiball bonus.
    // Balls are only ever APPENDED -- a lost ball is deactivated in place, never erased -- so the
    // index stamped into the collider's owner slot stays valid for the level's lifetime, and
    // m_balls stays parallel to li.balls.
    level_info& li = m.get_level_info();

    ball_state ball;
    ball.kind = rs::ke_ball_kind::ordinary;
    ball.size = 0; // 0x2C560 creates the smallest ordinary ball
    const auto frame = rs::require_ke_assets().balls.require_frame_rect(
        rs::ke_ball_frame(ball.kind, ball.size));
    ball.half = std::max(frame.w, frame.h) / 2;
    ball.pos = pos;
    ball.vel = vel;
    ball.active = true;

    const auto hs = static_cast <float>(ball.half);
    neutrino::physics::bullet body;
    body.shape = circle(pos.x - hs, pos.y - hs, pos.x + hs, pos.y + hs);
    body.velocity = neutrino::physics::vec{vel.x, vel.y}; // the one untyped crossing: a struct field
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

    const auto index = static_cast <std::uint32_t>(li.balls.size());
    body.user_data = neutrino::physics::to_user_data(ke_owner{ke_kind::ball, index});

    const auto eid = static_cast <neutrino::physics::entity_id_t>(EID_BALL_BASE + index);
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
        body.user_data = neutrino::physics::to_user_data(ke_owner{ke_kind::wall, 0});
        m_world.add(EID_WALL, body);
    }
}

void game_mechanics::build_bricks(const model& m) {
    const auto& assets = rs::require_ke_assets();
    // Bricks: one static body per domain brick, owning its index; the handle is kept parallel so
    // the brick can be removed on death without the model knowing collider ids exist.
    const level_info& li = m.get_level_info();
    m_brick_colliders.resize(li.bricks.size());
    for (std::size_t i = 0; i < li.bricks.size(); ++i) {
        const brick& b = li.bricks[i];
        if (b.m != brick::motion::ALIVE) {
            continue; // rebuilding after a lost life must not resurrect destroyed bricks
        }
        neutrino::rect tr{};
        if (b.frame >= 0) {
            if (const auto r = assets.bricks.frame_rect(static_cast <std::size_t>(b.frame))) {
                tr = *r;
            }
        }
        neutrino::physics::static_body body;
        body.shape = box(b.pos.x, b.pos.y, b.pos.x + static_cast <float>(tr.w),
                         b.pos.y + static_cast <float>(tr.h));
        body.user_data = neutrino::physics::to_user_data(ke_owner{ke_kind::brick, static_cast <std::uint32_t>(i)});
        m_brick_colliders[i] = m_world.add(static_cast <neutrino::physics::entity_id_t>(i), body);
    }
}

void game_mechanics::build_paddle(const model& m) {
    // Paddle: a kinematic body sized to its current form, re-synced each frame in tick().
    const paddle_info& p = m.get_paddle();
    neutrino::physics::kinematic_body body;
    body.shape = box(static_cast <float>(p.x), static_cast <float>(p.y),
                     static_cast <float>(p.x + p.w), static_cast <float>(p.y + p.h));
    body.user_data = neutrino::physics::to_user_data(ke_owner{ke_kind::paddle, 0});
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
    const auto cx = static_cast <float>(p.target_x); // target_x is the desired paddle centre
    const auto cy = static_cast <float>(p.y) + static_cast <float>(p.h) * 0.5f;
    m_world.set_target(m_paddle, neutrino::world_pos{cx, cy});
}

void game_mechanics::handle_balls(model& m, const neutrino::physics::world_event& e) {
    level_info& li = m.get_level_info();
    const auto& assets = rs::require_ke_assets();

    const std::size_t bi = m_world.owner_as <ke_owner>(e.mover).index;
    if (bi >= li.balls.size() || !li.balls[bi].active) {
        return;
    }
    ball_state& ball = li.balls[bi];
    auto spawn_hit = [&](neutrino::sprite_animation_id anim, const rs::ke_anim& src) {
        const auto r = static_cast <float>(ball.half);
        const neutrino::rect fr = assets.balls.require_frame_rect(src.frames[0]);
        // From the ball's centre: back along the contact normal to the ball's surface, then up
        // and left by half the frame, because KE_SPELL pivots top-left. Both are OFFSETS, so
        // they add to a position and could not be mistaken for one.
        const neutrino::world_delta to_surface{-e.normal.x() * r, -e.normal.y() * r};
        const neutrino::world_delta to_top_left{
            -static_cast <float>(fr.w) * 0.5f, -static_cast <float>(fr.h) * 0.5f
        };
        li.effects.push_back({
            m_world.position_of(m_balls[bi]) + to_surface + to_top_left,
            neutrino::create_sprite_state(anim)
        });
    };

    auto spawn_capsule = [&](const brick& b) {
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
    // handle is stale and reading its owner would abort (taking the whole gameplay scene down with
    // it). The bounce still physically happened against the brick during the sweep, so reflect the
    // ball; just do not damage, re-score, or re-sound a brick that is already dead.
    if (!m_world.is_valid(e.target)) {
        ball.vel = reflect(ball.vel, e.normal);
        m_world.set_velocity(m_balls[bi], ball.vel);
        return;
    }

    const ke_owner tgt = m_world.owner_as <ke_owner>(e.target);
    if (tgt.kind == ke_kind::paddle) {
        const float ball_x = m_world.position_of(m_balls[bi]).x;
        paddle_info& p = m.get_paddle();
        ball.vel = paddle_bounce(ball_x, p, p.w, ball.vel);
        ke::audio::instance().play(rs::ke_sfx::bounce_racket);
    } else {
        ball.vel = reflect(ball.vel, e.normal); // wall or brick
        if (tgt.kind == ke_kind::brick && tgt.index < li.bricks.size()) {
            spawn_hit(assets.hit_brick_anim_id, rs::hit_brick_anim);
            brick& b = li.bricks[tgt.index];
            if (b.hits == -1) {
                ke::audio::instance().play(rs::ke_sfx::brick_metal); // indestructible
            } else if (b.m == brick::motion::ALIVE && --b.hits <= 0) {
                // Decode already rejects unsupported TAB codes; retain the bound check
                // for bricks constructed by debug tools or future generators.
                if (b.bonus != rs::bonus::none
                    && static_cast <std::size_t>(b.bonus) < rs::ke_spell_capsule_anim.size()) {
                    spawn_capsule(b);
                }
                m.add_score(1); // original brick score: 1 << score_shift
                b.m = brick::motion::FLUNG;
                b.vel = scaled(ball.vel, ke_fling_speed); // fly off the way the ball went
                m_world.remove(m_brick_colliders[tgt.index]);
                m_brick_colliders[tgt.index] = {};
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
            ke::audio::instance().play(rs::ke_sfx::bounce_wall); // ke_kind::wall
        }
    }
    m_world.set_velocity(m_balls[bi], ball.vel);
}

void game_mechanics::apply_bonus(model& m, rs::bonus b, int mag) {
    LOG_INFO("Bonus:", b);
    level_info& li = m.get_level_info();
    auto& sfx = ke::audio::instance();
    const int strength = mag > 0 ? mag : 1; // bonus_mag is 1..4; guard a stray 0

    // 0x2C5BC adjusts each component by 4*mag in 1/16-pixel-per-tick units.
    // Convert once to px/s; preserve horizontal zero and the original axis limits.
    auto change_ball_speed = [&](int delta) {
        constexpr float unit = 70.0f / 16.0f;
        const auto component = [&](float v, float limit, bool keep_zero) {
            if (v == 0.0f && keep_zero) {
                return v;
            }
            const float next = std::clamp(std::abs(v) + 4.0f * delta * unit,
                                          4.0f * unit, limit * unit);
            return v > 0.0f ? next : -next;
        };
        for (std::size_t i = 0; i < li.balls.size(); ++i) {
            ball_state& ball = li.balls[i];
            if (ball.active) {
                ball.vel = {component(ball.vel.x, 32.0f, true),
                            component(ball.vel.y, 40.0f, false)};
                m_world.set_velocity(m_balls[i], ball.vel);
            }
        }
    };

    // IDs 4/8 change the sprite SIZE byte, not velocity. Keep the physical
    // shape in sync so the larger/smaller ball also changes its collision area.
    auto resize_balls = [&](int delta) {
        const auto& assets = rs::require_ke_assets();
        for (std::size_t i = 0; i < li.balls.size(); ++i) {
            ball_state& ball = li.balls[i];
            if (ball.active) {
                ball.size = std::clamp(ball.size + delta, 0, rs::ke_ball_size_count - 1);
                const auto frame = assets.balls.require_frame_rect(rs::ke_ball_frame(ball.kind, ball.size));
                ball.half = std::max(frame.w, frame.h) / 2;
                const auto r = static_cast <float>(ball.half);
                m_world.set_shape(m_balls[i], circle(ball.pos.x - r, ball.pos.y - r,
                                                   ball.pos.x + r, ball.pos.y + r));
            }
        }
    };

    // Grow/shrink the paddle by `delta` size-steps: update the model form AND the collider,
    // keeping the centre fixed and clamping the new box inside the pillars.
    auto resize_paddle = [&](int delta) {
        paddle_info& p = m.get_paddle();
        const int new_size = std::clamp(p.size + delta, 1, rs::ke_paddle_size_count);
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
        case rs::bonus::shield:
            m.get_paddle().shield += strength;
            sfx.play(rs::ke_sfx::bonus_good);
            break;
        case rs::bonus::damage_paddle:
            damage_paddle(m);
            break;
        case rs::bonus::clear_enemies:
            for (auto& enemy : li.enemies) {
                kill_enemy(m, enemy);
            }
            sfx.play(rs::ke_sfx::bonus_dyna);
            break;
        case rs::bonus::shrink_balls:
            resize_balls(-strength);
            sfx.play(rs::ke_sfx::bonus_minus);
            break;
        case rs::bonus::enlarge_balls:
            resize_balls(+strength);
            sfx.play(rs::ke_sfx::bonus_plus);
            break;
        case rs::bonus::slow_all_balls:
            change_ball_speed(-strength);
            sfx.play(rs::ke_sfx::bonus_minus);
            break;
        case rs::bonus::speed_up_all_balls:
            change_ball_speed(+strength);
            sfx.play(rs::ke_sfx::bonus_plus);
            break;
        case rs::bonus::extra_ball: {
            const auto live = std::count_if(li.balls.begin(), li.balls.end(),
                                           [](const ball_state& ball) { return ball.active; });
            if (live < static_cast <int>(ke_max_balls)) {
                const auto& p = m.get_paddle();
                // Original velocity (10,-20) / 16 pixels per 70 Hz tick.
                spawn_ball(m, {p.x + p.w * 0.5f, p.y - 4.0f}, {43.75f, -87.5f});
            }
            sfx.play(rs::ke_sfx::bonus_good);
            break;
        }
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
            sfx.play(rs::ke_sfx::racket_birth);
            break;
        case rs::bonus::score_multiplier:
            m.increase_score_multiplier(strength);
            sfx.play(rs::ke_sfx::bonus_jao);
            break;
        default:
            // Mapping is known; these mechanics still need implementation (docs/bonuses.md).
            LOG_DEBUG("Bonus effect not implemented:", b);
            break;
    }
    // The original awards this AFTER dispatch, so an x2 pickup uses its new multiplier.
    if (b != rs::bonus::none) {
        m.add_score(2);
    }
}

void game_mechanics::kill_enemy(model& m, enemy_state& enemy) {
    if (!enemy.alive) {
        return;
    }
    const auto bounds = enemy_box(enemy);
    enemy.pos = {(bounds.min.x() + bounds.max.x()) * 0.5f,
                 (bounds.min.y() + bounds.max.y()) * 0.5f + 5.0f};
    enemy.alive = false;
    enemy.animation_ticks = 0;
    enemy.hits = 0;
    m.add_score(3);
    ke::audio::instance().play(rs::ke_sfx::enemy_death);
}

void game_mechanics::lose_life(model& m) {
    auto& li = m.get_level_info();
    if (li.paddle_death_ticks >= 0) {
        return; // one life per death, even if several demons overlap
    }
    li.paddle_death_ticks = 0;
    m_enemy_clock = 0.0;
    m.add_life(-1);
    ke::audio::instance().play(rs::ke_sfx::racket_death);
}

void game_mechanics::damage_paddle(model& m) {
    if (m.get_level_info().paddle_death_ticks >= 0) {
        return;
    }
    ke::audio::instance().play(rs::ke_sfx::bonus_minus);
    if (m.get_paddle().shield > 0) {
        --m.get_paddle().shield;
    } else {
        lose_life(m);
    }
}

void game_mechanics::tick_enemies(model& m) {
    auto& li = m.get_level_info();
    ++li.animation_ticks;
    const auto& level = rs::require_ke_assets().levels[m.get_level()];
    constexpr auto step = neutrino::sim_duration{1.0f / 70.0f};

    if (li.hatch_ticks >= 0 && ++li.hatch_ticks >= rs::enemy_hatch_anim.count * rs::enemy_hatch_anim.ticks) {
        li.hatch_ticks = -1;
    }
    if (level.spawn_period.count() > 0) {
        if (m_spawn_ticks == 55) {
            li.hatch_ticks = 0;
        }
        if (--m_spawn_ticks <= 0) {
            m_spawn_ticks = level.spawn_period.count();
            const auto type = level.spawn_seq[m_spawn_index++ % level.spawn_seq.size()];
            const auto count = std::count_if(li.enemies.begin(), li.enemies.end(),
                                            [](const enemy_state& e) { return e.alive; });
            if (count < 8) {
                enemy_state enemy;
                enemy.type = static_cast <rs::enemy>(std::min(static_cast <unsigned>(type), 7u));
                enemy.pos = {160.0f, 16.0f};
                enemy.vel = {(m_enemy_rng() & 0x100u) ? 70.0f : -70.0f, 70.0f};
                enemy.turn_ticks = std::max(1, static_cast <int>((m_enemy_rng() & 0xFFFFu) >> 7));
                enemy.hits = m.get_level() / 20 + 2;
                li.enemies.push_back(enemy);
                ke::audio::instance().play(rs::ke_sfx::enemy_create);
            }
        }
    }

    for (auto& enemy : li.enemies) {
        enemy.pos += enemy.vel * step;
        if (!enemy.alive) {
            ++enemy.animation_ticks;
            continue;
        }
        const auto& anim = rs::enemy_anim(enemy.type);
        enemy.animation_ticks = (enemy.animation_ticks + 1) % (anim.count * anim.ticks);
        if (--enemy.turn_ticks <= 0) {
            const auto random = m_enemy_rng();
            enemy.turn_ticks = std::max(1, static_cast <int>((random & 0xFFFFu) >> 7));
            if (random & 0x20u) {
                enemy.vel.x = -enemy.vel.x;
            } else if (random & 8u) {
                enemy.vel.y = -enemy.vel.y;
            }
        }
        const auto bounds = enemy_box(enemy);
        if ((bounds.min.x() <= 16.0f && enemy.vel.x < 0.0f)
            || (bounds.max.x() >= 304.0f && enemy.vel.x > 0.0f)) {
            enemy.vel.x = -enemy.vel.x;
        }
        if ((bounds.min.y() <= 24.0f && enemy.vel.y < 0.0f)
            || (bounds.max.y() >= 200.0f && enemy.vel.y > 0.0f)) {
            enemy.vel.y = -enemy.vel.y;
        }

        for (std::size_t i = 0; i < li.balls.size(); ++i) {
            auto& ball = li.balls[i];
            if (!ball.active || ball.kind == rs::ke_ball_kind::ghost) {
                continue;
            }
            const float r = static_cast <float>(ball.half);
            if (!neutrino::physics::intersects(
                    box(ball.pos.x - r, ball.pos.y - r, ball.pos.x + r, ball.pos.y + r),
                    enemy_box(enemy, 3))) {
                continue;
            }
            // The BALL path (0x2BAF0) kills outright. The 1/4 HP damage table
            // in 0x2CB00 belongs to fired projectiles, not ordinary balls.
            enemy.vel.x += std::floor(ball.vel.x / 70.0f) * 70.0f;
            enemy.vel.y += std::floor(ball.vel.y / 70.0f) * 70.0f;
            kill_enemy(m, enemy);
            if (ball.kind != rs::ke_ball_kind::power) {
                if (m_enemy_rng() & 0x10u) {
                    ball.vel.y = -ball.vel.y;
                } else {
                    ball.vel.x = -ball.vel.x;
                }
                if (ball.vel.x == 0.0f) {
                    ball.vel.x = -17.5f;
                } else {
                    const float delta = (m_enemy_rng() & 1u) ? -35.0f : 35.0f;
                    ball.vel.x = std::copysign(std::clamp(std::abs(ball.vel.x) + delta, 17.5f, 140.0f), ball.vel.x);
                }
                m_world.set_velocity(m_balls[i], ball.vel);
            }
            break;
        }
        const auto& paddle = m.get_paddle();
        const auto paddle_bounds = box(paddle.x + 3.0f, paddle.y + 3.0f,
                                       paddle.x + paddle.w - 5.0f, paddle.y + paddle.h - 5.0f);
        if (enemy.alive && neutrino::physics::intersects(bounds, paddle_bounds)) {
            if (enemy.type == rs::enemy::ship_demon) {
                const bool shielded = m.get_paddle().shield > 0;
                damage_paddle(m);
                if (!shielded) {
                    return; // lethal demon survives until the life reset
                }
            }
            kill_enemy(m, enemy); // normal enemies are squashed; a shield also squashes demons
        }
    }
    std::erase_if(li.enemies, [](const enemy_state& enemy) {
        return !enemy.alive && enemy.animation_ticks >= rs::enemy_death_anim.count * rs::enemy_death_anim.ticks;
    });
}

void game_mechanics::tick(model& m, neutrino::sim_duration dt) {
    level_info& li = m.get_level_info();
    paddle_info& p = m.get_paddle();

    if (li.paddle_death_ticks >= 0) {
        m_enemy_clock += dt.count();
        const int ticks = static_cast <int>(m_enemy_clock * 70.0 + 1e-6);
        m_enemy_clock -= ticks / 70.0;
        li.paddle_death_ticks = std::min(li.paddle_death_ticks + ticks, 24);
        if (li.paddle_death_ticks >= 24 && m.get_lives() > 0) {
            m.reset_paddle();
            load(m); // retain the surviving brick grid and score; clear actors/effects
            ke::audio::instance().play(rs::ke_sfx::racket_birth);
        }
        return;
    }

    handle_paddle(m);

    // 2. Always step the world: move-and-slide (inside run) is what actually applies the paddle's
    //    velocity, so the paddle keeps reacting while the last capsules fall, even with no ball
    //    in play. Ball handling no-ops on inactive/removed balls. Each ball is a
    //    bullet, so e.mover identifies which ball hit (one hit per ball per frame at most).
    //
    //    No activity region: the whole playfield is always live. This used to pass a 320x200 box
    //    -- the screen -- which was a full-world region spelled as if it were a camera view, and
    //    the two only coincide because KE has no camera.
    const auto& events = m_world.run(dt.count());

    // The paddle move-and-slid this step (a wall may have stopped it short); read its resolved
    // position back before handling ball hits -- the ball swept against it at that position. The
    // model holds the TOP-LEFT corner while set_target steers the CENTRE, so read the extent
    // rather than the centre: deriving one from the other would round-trip through a half-size
    // and could land a pixel out at an integer boundary.
    const neutrino::world_pos paddle_top_left = m_world.bounds_of(m_paddle).min;
    p.x = static_cast <int>(paddle_top_left.x);
    p.y = static_cast <int>(paddle_top_left.y);

    for (const neutrino::physics::world_event& e : events) {
        if (e.kind != neutrino::physics::event_kind::BULLET_HIT) {
            continue;
        }
        if (m_world.owner_as <ke_owner>(e.mover).kind != ke_kind::ball) {
            continue; // the only bullets today are balls, but say so rather than assume it
        }
        handle_balls(m, e);
    }

    // 3. Read each ball's position back (for drawing) and drop balls that fell out.
    for (std::size_t i = 0; i < li.balls.size(); ++i) {
        ball_state& ball = li.balls[i];
        if (!ball.active) {
            continue;
        }
        ball.pos = m_world.position_of(m_balls[i]);
        if (ball.pos.y - static_cast <float>(ball.half) > static_cast <float>(m_bottom_margin)) {
            ball.active = false; // fell past the paddle -> lost
            m_world.remove(m_balls[i]);
            m_balls[i] = {};
        }
    }

    m_enemy_clock += dt.count();
    while (m_enemy_clock + 1e-9 >= 1.0 / 70.0) {
        m_enemy_clock -= 1.0 / 70.0;
        tick_enemies(m);
        if (li.paddle_death_ticks >= 0) {
            return;
        }
    }

    // 4. Slide any flung bricks (off-screen culling comes with the dynamic-brick rendering).
    //    A rate integrated over the step yields an offset, which displaces a position -- the
    //    types say so, and a stray `pos += vel` or `pos += vel * vel` no longer compiles.
    for (brick& b : li.bricks) {
        if (b.m == brick::motion::FLUNG) {
            b.pos += b.vel * dt;
        }
    }

    // 5. Slide bonus capsules
    for (auto& cap : li.capsules) {
        cap.pos += ke_capsule_fall * dt;
        if (neutrino::physics::intersects(cap.box(), p.box())) {
            apply_bonus(m, cap.bonus, cap.mag);
            cap.active = false;
            if (li.paddle_death_ticks >= 0) {
                break;
            }
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
    if (li.capsules.empty() && std::none_of(li.balls.begin(), li.balls.end(),
                                          [](const ball_state& ball) { return ball.active; })) {
        lose_life(m);
    }
}
