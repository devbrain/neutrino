// Integration checks against real BOB/TAB resources. Run tools/test_enemies.py.
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <neutrino/application.hh>
#include <neutrino/video/globals.hh>
#include <sdlpp/app/entry_point.hh>
#include <ke/assets/registry.hh>
#include <ke/assets/sprites.hh>
#include <ke/game/mechanics.hh>
#include <ke/game/model.hh>
#include <ke/scenes/play_game_scene.hh>

namespace {
    void check(bool ok, const char* message) {
        if (!ok) throw std::runtime_error(message);
    }
    class enemy_tests : public neutrino::application {
        rs::game_resources resources;
        std::unique_ptr<rs::ke_assets> assets;
        game_mechanics mechanics;

        static neutrino::application_config config() {
            neutrino::application_config c;
            c.title = "KE enemy tests"; c.width = 320; c.height = 200;
            c.logical_size = neutrino::dim{320, 200};
            return c;
        }
        void step(int ticks = 1) {
            auto& m = model::instance();
            for (int i = 0; i < ticks; ++i) {
                // Keep the test paddle under the ball while checking long spawn intervals.
                if (!m.get_level_info().balls.empty())
                    m.set_paddle_target(static_cast<int>(m.get_level_info().balls[0].pos.x));
                mechanics.tick(m, neutrino::sim_duration{1.0f / 70.0f});
            }
        }
        void reset(int period = 0) {
            auto& level = assets->levels[0];
            level.spawn_period = rs::ke_tick{period};
            for (auto& cell : level.cells) cell = {};
            for (std::size_t i = 0; i < level.spawn_seq.size(); ++i)
                level.spawn_seq[i] = static_cast<rs::enemy>(i);
            auto& m = model::instance();
            m.restart_game(); mechanics.load(m);
        }
        void collect(rs::bonus bonus, int magnitude = 1) {
            auto& m = model::instance();
            const auto& p = m.get_paddle();
            capsule c{};
            c.bonus = bonus; c.mag = magnitude; c.active = true;
            c.pos = {static_cast<float>(p.x + p.w/2), static_cast<float>(p.y)};
            c.w = 12; c.h = 12;
            c.state = neutrino::create_sprite_state(assets->capsule_anim_id[static_cast<std::size_t>(bonus)]);
            m.get_level_info().capsules.push_back(c);
            mechanics.tick(m, neutrino::sim_duration{0.0001f});
            check(m.get_level_info().capsules.empty(), "test pickup was not collected");
        }
        void ready() override {
            const char* path = std::getenv("KE_TEST_RSC");
            std::ifstream input(path ? path : "/home/igor/games/ke/Krypton-Egg_DOS_EN/ke.rsc", std::ios::binary);
            auto parsed = rs::parse(input);
            check(parsed.has_value(), "cannot load game resources");
            resources = std::move(*parsed);
            assets = std::make_unique<rs::ke_assets>();
            assets->levels = resources.levels;
            assets->m_resources = &resources;
            rs::set_ke_assets(*assets); rs::define_sprites(resources);
            auto& m = model::instance();
            auto& li = m.get_level_info();

            // Logical bounds use the original signed BOB offset, not its negation.
            const auto frame0 = assets->enemies.require_metrics(0).local_bounds();
            check(frame0.x == -23 && frame0.y == -4 && frame0.w == 47, "BOB enemy anchor sign");
            const auto demon = assets->enemies.require_metrics(65).local_bounds();
            check(demon.x == -13 && demon.y == -8 && demon.h == 38, "demon changing bounds");

            reset(500);
            li.balls[0].kind = rs::ke_ball_kind::ghost;
            step(445); check(li.enemies.empty() && li.hatch_ticks == -1, "hatch opened too soon");
            step(); check(li.hatch_ticks == 0, "hatch missing 55 ticks before spawn");
            step(53); check(li.enemies.empty(), "spawned before period");
            step(); check(li.enemies.size() == 1 && li.enemies[0].type == rs::enemy::insectoid, "first scheduled spawn");
            check(std::abs(li.enemies[0].pos.x - 160) == 1 && li.enemies[0].pos.y == 17, "spawn anchor/first movement");
            check(li.enemies[0].hits == 2, "initial enemy health");

            reset(2); li.balls[0].kind = rs::ke_ball_kind::ghost;
            step(16); check(li.enemies.size() == 8, "eight-enemy spawn limit");
            for (int i=0; i<8; ++i) check(static_cast<int>(li.enemies[i].type)==i, "TAB spawn sequence");
            step(2); check(li.enemies.size()==8, "spawn cap exceeded");
            li.enemies.erase(li.enemies.begin());
            step(2); check(li.enemies.back().type==rs::enemy::green_alien, "sequence must advance even when capped");
            assets->levels[0].spawn_period = rs::ke_tick{0};
            auto& e = li.enemies[0];
            e.type = rs::enemy::ufo_disc; e.animation_ticks=0; e.pos={24,80}; e.vel={-70,0}; e.turn_ticks=100;
            step(); check(e.vel.x==70, "left wall bounce");
            e.pos={296,80}; e.vel={70,0}; step(); check(e.vel.x==-70, "right wall bounce");
            e.pos={100,24}; e.vel={0,-70}; step(); check(e.vel.y==70, "top bounce");
            e.pos={100,184}; e.vel={0,70}; step(); check(e.vel.y==-70, "bottom bounce");
            e.pos={100,80}; e.vel={0,0}; e.turn_ticks=1; step(); check(e.turn_ticks>=1 && e.turn_ticks<=511, "turn timer reload");
            e.type=rs::enemy::ship_demon; e.animation_ticks=0;
            for(int i=0;i<33;++i) { check(e.frame()==rs::enemy_anim(e.type).frames[e.animation_ticks/3], "demon frame sequence"); step(); }
            check(e.animation_ticks==0, "enemy animation loop");

            reset();
            step(15); // test ball contacts clear of the paddle's squash box
            enemy_state victim; victim.type=rs::enemy::ufo_disc; victim.turn_ticks=1000;
            victim.pos={li.balls[0].pos.x, li.balls[0].pos.y-8};
            li.enemies.push_back(victim);
            const auto before=li.balls[0].vel;
            step(); check(!li.enemies[0].alive && m.get_score()==3, "ball must kill/score once");
            check(li.balls[0].vel.x!=before.x || li.balls[0].vel.y!=before.y, "ordinary ball must deflect");
            step(42); check(li.enemies.empty() && m.get_score()==3, "death animation cleanup/duplicate score");

            reset(); step(15); li.balls[0].kind=rs::ke_ball_kind::ghost;
            victim.pos={li.balls[0].pos.x, li.balls[0].pos.y-8}; li.enemies.push_back(victim);
            step(); check(li.enemies[0].alive, "ghost ball must ignore enemies");
            reset(); step(15); li.balls[0].kind=rs::ke_ball_kind::power;
            victim.pos={li.balls[0].pos.x, li.balls[0].pos.y-8}; li.enemies.push_back(victim);
            const auto powered=li.balls[0].vel;
            step(); check(!li.enemies[0].alive && li.balls[0].vel.x==powered.x && li.balls[0].vel.y==powered.y, "power ball must not deflect");

            reset();
            victim.pos={m.get_paddle().x+m.get_paddle().w*0.5f, static_cast<float>(m.get_paddle().y)};
            li.enemies.push_back(victim);
            step(); check(!li.enemies[0].alive && m.get_lives()==3, "paddle squashes ordinary enemy");
            reset(); collect(rs::bonus::shield,2);
            victim.type=rs::enemy::ship_demon;
            victim.pos={m.get_paddle().x+m.get_paddle().w*0.5f, static_cast<float>(m.get_paddle().y)};
            li.enemies.push_back(victim);
            step(); check(!li.enemies[0].alive && m.get_paddle().shield==1 && m.get_lives()==3, "shield/demon collision");
            reset();
            victim.pos={m.get_paddle().x+m.get_paddle().w*0.5f, static_cast<float>(m.get_paddle().y)};
            li.enemies.push_back(victim); li.enemies.push_back(victim);
            step(); check(m.get_lives()==2 && li.paddle_death_ticks==0, "multiple demons must cost one life");
            step(24); check(li.paddle_death_ticks==-1 && li.enemies.empty() && li.balls.size()==1, "life respawn/reset");

            reset();
            assets->levels[0].cells[0] = rs::ke_cell::decode(0x31, 0);
            assets->levels[0].cells[1] = rs::ke_cell::decode(0x31, 0);
            m.restart_game(); mechanics.load(m);
            li.bricks[0].m = brick::motion::FLUNG;
            m.add_score(7);
            collect(rs::bonus::damage_paddle); step(24);
            check(li.bricks.size()==2 && li.bricks[0].m==brick::motion::FLUNG
                  && li.bricks[1].m==brick::motion::ALIVE && m.get_score()==9,
                  "respawn must preserve destroyed bricks, surviving bricks and score");

            reset();
            m.set_paddle_target(20); // let the real ball fall through the open bottom
            for (int i=0;i<1500 && li.paddle_death_ticks<0;++i)
                mechanics.tick(m, neutrino::sim_duration{1.0f/120.0f});
            check(m.get_lives()==2 && li.paddle_death_ticks==0, "last ball falling must cost a life");

            reset();
            for(int i=0;i<8;++i) { enemy_state target; target.type=static_cast<rs::enemy>(i); target.pos={float(40+i*30),70}; li.enemies.push_back(target); }
            collect(rs::bonus::clear_enemies);
            check(std::none_of(li.enemies.begin(),li.enemies.end(),[](const enemy_state& x){return x.alive;}) && m.get_score()==26, "dynamite and enemy/pickup score");
            for (int i=0;i<3;++i) { collect(rs::bonus::damage_paddle); step(24); }
            check(m.get_lives()==0 && li.paddle_death_ticks==24, "game over");
            m.restart_game(); mechanics.load(m);
            check(m.get_lives()==3 && m.get_score()==0 && li.enemies.empty(), "new game reset");

            // Exercise the actual scene renderer with all eight frame families.
            play_game_scene scene(*assets); scene.on_enter();
            for (int i=0;i<8;++i) { enemy_state target; target.type=static_cast<rs::enemy>(i); target.pos={float(52+i%4*72),float(55+i/4*65)}; target.animation_ticks= i==2 ? 21 : 0; li.enemies.push_back(target); }
            li.hatch_ticks=55; m.get_paddle().shield=2; scene.render();
            if (const char* screenshot=std::getenv("KE_TEST_SCREENSHOT")) {
                SDL_Surface* surface=SDL_RenderReadPixels(neutrino::get_renderer().get(),nullptr);
                check(surface!=nullptr,"cannot capture frame");
                const bool saved=SDL_SaveBMP(surface,screenshot); SDL_DestroySurface(surface);
                check(saved,"cannot save frame");
            }
            m.add_life(-m.get_lives()); li.paddle_death_ticks=24;
            scene.render(); // exercise game-over text as well as the sprite renderer
            scene.fixed_update(neutrino::sim_duration{1.0f/120.0f},
                               neutrino::input_snapshot{{}, {true,false,true}, {}, {}});
            check(m.get_lives()==3 && m.get_score()==0 && li.paddle_death_ticks==-1
                  && li.enemies.empty() && li.balls.size()==1, "game-over click must restart gameplay");
            scene.on_exit();
            std::puts("PASS: enemy schedule/cap/sequence, BOB bounds, wall turns, animations, ball/paddle combat, shields, dynamite, death cleanup, respawn/restart, rendering.");
            quit();
        }
        std::unique_ptr<neutrino::base_scene> create_initial_scene() override { return nullptr; }
        void teardown() override {
            if(assets) { model::instance().get_level_info().clear(); rs::release_sprites(); rs::clear_ke_assets(); assets.reset(); }
        }
    public:
        enemy_tests() : application(config()) {}
    };
}
SDLPP_MAIN(enemy_tests)
