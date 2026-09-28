//
// KE sound-effect event table: game event -> KE_MAIN.DIG sample index (tab.md §8).
// The engine plays SFX by index; "pool" events span a range of interchangeable variations.
//

#pragma once

#include <cstdint>

namespace rs {

    // Gameplay sound events (tab.md §8), corroborated by the reverse-engineered handlers.
    enum class ke_sfx {
        brick_slide,    // a multi-hit brick takes a hit (downgrade)
        brick_break,    // brick destroyed
        brick_metal,    // hit on a metal / indestructible brick
        brick_pan,      // hit on a "pan" material brick
        bonus_bad,      // malus collected
        bonus_good,     // good bonus collected
        bonus_minus,    // shrink
        bonus_plus,     // enlarge
        bonus_create,   // capsule spawned (original call uses one-based sample 29)
        bonus_glue,     // catch / glue
        bonus_jao,
        bonus_dyna,     // dynamite
        bonus_fly,      // flying paddle
        racket_death,   // paddle destroyed / life lost
        racket_birth,   // new paddle / start of life
        bounce_fire,    // fireball (through-ball) bounce
        bounce_spectre, // ball bounces off a "spectre"
        bounce_wall,    // ball bounces off a wall
        bounce_racket,  // ball bounces off the paddle
        enemy_create,   // enemy spawned
        enemy_death,    // enemy destroyed
        fire_canon,     // gun / cannon fires
        fire_single,    // single shot
        fire_double,    // double shot
        player_touched, // paddle hit by an enemy
    };

    // A contiguous span of KE_MAIN.DIG sample indices. count > 1 marks a variation *pool*
    // (tab.md §8: the engine picks one of [first, first+count) per occurrence).
    struct ke_sfx_range {
        uint16_t first;
        uint16_t count;
    };

    // KE_MAIN.DIG sample range for @p e (tab.md §8; comments are the designer's name1 ids).
    [[nodiscard]] constexpr ke_sfx_range ke_sfx_samples(ke_sfx e) noexcept {
        switch (e) {
            case ke_sfx::brick_slide:    return {0, 9};   // KESLID01..09
            case ke_sfx::brick_break:    return {9, 6};   // KEBREA01..06
            case ke_sfx::brick_metal:    return {15, 4};  // KEMETA01..04
            case ke_sfx::brick_pan:      return {19, 5};  // KEPAN01..05
            case ke_sfx::bonus_bad:      return {24, 1};  // KESPLBAD
            case ke_sfx::bonus_good:     return {25, 1};  // KESPLGOD
            case ke_sfx::bonus_minus:    return {26, 1};  // KESPLMIN
            case ke_sfx::bonus_plus:     return {27, 1};  // KESPLPLU
            case ke_sfx::bonus_create:   return {28, 1};  // KESPLCRE
            case ke_sfx::bonus_glue:     return {29, 1};  // KESPLGLU
            case ke_sfx::bonus_jao:      return {30, 1};  // KESPLJAO
            case ke_sfx::bonus_dyna:     return {31, 1};  // KESPLDYN
            case ke_sfx::bonus_fly:      return {32, 1};  // KESPLFLY
            case ke_sfx::racket_death:   return {33, 1};  // KERAKDET
            case ke_sfx::racket_birth:   return {34, 1};  // KERAKBIR
            case ke_sfx::bounce_fire:    return {35, 1};  // KEBNCFIR
            case ke_sfx::bounce_spectre: return {36, 1};  // KEBNCSPE
            case ke_sfx::bounce_wall:    return {37, 1};  // KEBNCWAL
            case ke_sfx::bounce_racket:  return {38, 1};  // KEBNCRAK
            case ke_sfx::enemy_create:   return {39, 1};  // KENMYCRE
            case ke_sfx::enemy_death:    return {40, 1};  // KENMYDET
            case ke_sfx::fire_canon:     return {41, 1};  // KEFIRCAN
            case ke_sfx::fire_single:    return {42, 1};  // KEFIRSGL
            case ke_sfx::fire_double:    return {43, 1};  // KEFIRDBL
            case ke_sfx::player_touched: return {44, 1};  // KEPLRTOU
        }
        return {0, 1};
    }

    // The DIG bank holding the gameplay SFX (the other .dig banks are per-screen tracks).
    inline constexpr const char* ke_sfx_bank = "ke_main.dig";

} // namespace rs
