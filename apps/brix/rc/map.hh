//
// Created by igor on 23/09/2020.
//

#ifndef NEUTRINO_MAP_HH
#define NEUTRINO_MAP_HH

#include <vector>

enum class map_item_t
{
    EMPTY,

    BLOCK1,
    BLOCK2,

    GEM1,
    GEM2,
    GEM3,
    GEM4,
    GEM5,
    GEM6,
    GEM7,
    GEM8,

    DESTRUCTABLE,
    PLATFORM_V,

    PORTAL1,
    PORTAL2,
    PORTAL3,
    PORTAL4,
    PORTAL5,
    PORTAL6,
    PORTAL7,
    PORTAL8,
    PORTAL9,
    PORTAL10,
    CLOCK,

    UNDEFINED
};

struct problem_descr
{
  int level;
  int problem;
  int subproblem;
};

struct level_map
{
  int w;
  int h;
  int hot_x;
  int hot_y;
  int mins;
  int secs;
  problem_descr problem;
  std::vector<map_item_t> tiles;
};

#endif //NEUTRINO_MAP_HH
