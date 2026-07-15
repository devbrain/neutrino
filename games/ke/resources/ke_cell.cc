//
// Compile-time verification of the ke_cell decode against the tab.md §4/§5
// tables. ke_cell::decode is constexpr, so these static_asserts are the test
// suite for this module (the standalone `ke` target has no doctest harness).
//

#include "resources/ke_cell.hh"

namespace rs {

    // -- empty ----------------------------------------------------------------
    static_assert(ke_cell::decode(0x00, 0x00).kind == brick_kind::empty);
    static_assert(ke_cell::decode(0x00, 0x00).is_empty());
    static_assert(ke_cell::decode(0x00, 0x00).graphic == -1);

    // -- graphic off-by-one (KE_BRICK block = tile_id - 1) --------------------
    static_assert(ke_cell::decode(0x01, 0x00).graphic == 0x00);
    static_assert(ke_cell::decode(0x90, 0x00).graphic == 0x8F);
    static_assert(ke_cell::decode(0x91, 0x00).graphic == 0x90); // still drawn
    static_assert(ke_cell::decode(0xFF, 0x00).graphic == 0xFE);

    // -- durability tiers (high nibble within a group of 0x30) ----------------
    static_assert(ke_cell::decode(0x01, 0).kind == brick_kind::single_hit);
    static_assert(ke_cell::decode(0x01, 0).hits == 1);
    static_assert(ke_cell::decode(0x10, 0).hits == 1); // tier 1 upper bound
    static_assert(ke_cell::decode(0x11, 0).kind == brick_kind::multi_hit);
    static_assert(ke_cell::decode(0x11, 0).hits == 2);
    static_assert(ke_cell::decode(0x20, 0).hits == 2);
    static_assert(ke_cell::decode(0x21, 0).hits == 3);
    static_assert(ke_cell::decode(0x30, 0).hits == 3);
    // groups B and C repeat the same tier pattern.
    static_assert(ke_cell::decode(0x31, 0).hits == 1);
    static_assert(ke_cell::decode(0x41, 0).hits == 2);
    static_assert(ke_cell::decode(0x61, 0).hits == 1);
    static_assert(ke_cell::decode(0x71, 0).hits == 2);
    static_assert(ke_cell::decode(0x90, 0).hits == 3);
    static_assert(ke_cell::decode(0x01, 0).counts_toward_clear);
    static_assert(ke_cell::decode(0x90, 0).counts_toward_clear);

    // -- colour = low nibble, preserved across damage -------------------------
    static_assert(ke_cell::decode(0x2A, 0).colour == 0x0A);
    static_assert(ke_cell::decode(0x7C, 0).colour == 0x0C);

    // -- bonus drops only from group-B (0x31-0x60) with a nonzero bonus attr --
    static_assert(ke_cell::decode(0x41, 0x04).drops_bonus);
    static_assert(ke_cell::decode(0x41, 0x04).bonus_type == bonus::gun_laser); // 0x04 >> 2 = 1
    static_assert(ke_cell::decode(0x41, 0x04).bonus_mag == 1);        // (0x04 & 3) + 1
    static_assert(ke_cell::decode(0x41, 0x0F).bonus_type == bonus::open_exit); // 0x0F >> 2 = 3
    static_assert(ke_cell::decode(0x41, 0x0F).bonus_mag == 4);        // (0x0F & 3) + 1
    static_assert(!ke_cell::decode(0x41, 0x00).drops_bonus);          // no bonus attr
    static_assert(!ke_cell::decode(0x01, 0x04).drops_bonus);          // group A never drops
    static_assert(!ke_cell::decode(0x61, 0x04).drops_bonus);          // group C never drops
    static_assert(ke_cell::decode(0x01, 0x04).bonus_type == bonus::none); // non-dropping => none
    // type 0 is a REAL bonus (enlarge), not "none": attr>>2 = 0x20 masks to 0 but still drops.
    static_assert(ke_cell::decode(0x41, 0x80).drops_bonus);
    static_assert(ke_cell::decode(0x41, 0x80).bonus_type == bonus::enlarge_paddle);

    // -- high ranges ----------------------------------------------------------
    static_assert(ke_cell::decode(0x91, 0).kind == brick_kind::indestructible);
    static_assert(ke_cell::decode(0xF4, 0).kind == brick_kind::indestructible);
    static_assert(!ke_cell::decode(0x91, 0).counts_toward_clear);
    static_assert(ke_cell::decode(0xF5, 0).kind == brick_kind::trigger);
    static_assert(ke_cell::decode(0xF7, 0).kind == brick_kind::trigger);
    static_assert(ke_cell::decode(0xF8, 0).kind == brick_kind::marker);
    static_assert(ke_cell::decode(0xF9, 0).kind == brick_kind::marker);

    // -- special bricks carry remaining hit count in attr >> 2 ----------------
    static_assert(ke_cell::decode(0xFB, 0x0C).kind == brick_kind::special);
    static_assert(ke_cell::decode(0xFB, 0x0C).hits == 3);            // 0x0C >> 2
    static_assert(ke_cell::decode(0xFF, 0x28).hits == 0x0A);        // 0x28 >> 2
} // namespace rs
