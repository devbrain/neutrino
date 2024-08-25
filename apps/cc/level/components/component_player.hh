//
// Created by igor on 8/25/24.
//

#ifndef COMPONENT_PLAYER_HH
#define COMPONENT_PLAYER_HH

#include <sdlpp/video/geometry.hh>

struct component_player {
    component_player()
        : bullets(5),
          lives(3),
          score(0) {
    }

    int bullets;
    int lives;
    unsigned long score;
};

#endif
