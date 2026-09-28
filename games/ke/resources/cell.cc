//
// Compile-time verification against docs/bonuses.md and tab.md §5 (brick types).
// ke_cell::decode is constexpr, so these static_asserts are the test
// suite for this module (the standalone `ke` target has no doctest harness).
//

#include <ke/resources/cell.hh>

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
    static_assert(ke_cell::decode(0x41, 0x04).bonus_type == bonus::enlarge_paddle); // (4 >> 2) - 1 = 0
    static_assert(ke_cell::decode(0x41, 0x04).bonus_mag == 1);        // (0x04 & 3) + 1
    static_assert(ke_cell::decode(0x41, 0x0F).bonus_type == bonus::score_multiplier); // (15 >> 2) - 1 = 2
    static_assert(ke_cell::decode(0x41, 0x0F).bonus_mag == 4);        // (0x0F & 3) + 1
    static_assert(!ke_cell::decode(0x41, 0x00).drops_bonus);          // no bonus attr
    static_assert(!ke_cell::decode(0x01, 0x04).drops_bonus);          // group A never drops
    static_assert(!ke_cell::decode(0x61, 0x04).drops_bonus);          // group C never drops
    static_assert(ke_cell::decode(0x01, 0x04).bonus_type == bonus::none); // non-dropping => none
    static_assert(ke_cell::decode(0x31, 0x73).bonus_type == bonus::clear_enemies);
    static_assert(ke_cell::decode(0x31, 0x73).bonus_mag == 4);
    static_assert(!ke_cell::decode(0x41, 0x74).drops_bonus); // first rejected code
    static_assert(!ke_cell::decode(0x41, 0x80).drops_bonus); // no wrapping to ID 0
    static_assert(ke_cell::decode(0x41, 0x80).bonus_type == bonus::none);
    static_assert(ke_cell::decode(0x41, 0xFF).bonus_mag == 0);

    // Real KE_LDCWC.TAB level-1 fixtures (zero-based x,y): freeze at (2,0),
    // darkness at (1,1), flight at (0,2), extra ball at (7,5), life at (12,7).
    static_assert(ke_cell::decode(0x37, 0x3C).bonus_type == bonus::freeze_paddle);
    static_assert(ke_cell::decode(0x37, 0x28).bonus_type == bonus::darkness);
    static_assert(ke_cell::decode(0x37, 0x38).bonus_type == bonus::flying_paddle);
    static_assert(ke_cell::decode(0x33, 0x22).bonus_type == bonus::extra_ball);
    static_assert(ke_cell::decode(0x33, 0x22).bonus_mag == 3); // still only ONE extra ball
    static_assert(ke_cell::decode(0x35, 0x1C).bonus_type == bonus::extra_life);

    // Every attribute, including unused codes, on representative boundary tiles.
    // Mirrors the two original routines: destroy_brick extracts/subtracts, then
    // spawn_bonus rejects the unsigned byte if it is >= 28 before masking.
    constexpr bool verify_bonus_decode() {
        for (const auto tile : {0x00, 0x01, 0x30, 0x31, 0x40, 0x41, 0x60, 0x61, 0x90, 0xFA, 0xFF}) {
            for (int attr = 0; attr < 256; ++attr) {
                const auto c = ke_cell::decode(static_cast <std::uint8_t>(tile),
                                                static_cast <std::uint8_t>(attr));
                const auto raw_id = static_cast <std::uint8_t>((attr >> 2) - 1);
                const bool drops = tile >= 0x31 && tile <= 0x60 && (attr & 0xFC) && raw_id < 28;
                if (c.drops_bonus != drops
                    || c.bonus_type != (drops ? static_cast <bonus>(raw_id & 31) : bonus::none)
                    || c.bonus_mag != (drops ? (attr & 3) + 1 : 0)) {
                    return false;
                }
            }
        }
        return true;
    }
    static_assert(verify_bonus_decode());

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
