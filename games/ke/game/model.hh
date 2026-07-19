//
// Created by igor on 17/07/2026.
//

#pragma once

#include <neutrino/world/world.hh>
#include <neutrino/physics/collide/world.hh>

#include <ke/assets/sprites.hh>

struct paddle_info {
    int x;
    int y;
    int size;
    rs::ke_paddle_state state {rs::ke_paddle_state::simple};
};

class model {
    public:
        static model& instance();

        [[nodiscard]] int get_level() const;
        void load_level ();

        [[nodiscard]] const neutrino::world& get_visual_world() const;
        [[nodiscard]] const neutrino::physics::world& get_physical_world() const;

        neutrino::world& get_visual_world();
        neutrino::physics::world& get_physical_world();

        [[nodiscard]] const paddle_info& get_paddle() const;
        paddle_info& get_paddle();
    private:
        model();
    private:
        int m_level = 0;
        neutrino::world m_visual_world;
        neutrino::physics::world m_physical_world;
        paddle_info m_paddle;
};


