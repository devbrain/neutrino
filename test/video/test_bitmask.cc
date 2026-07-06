#include <doctest/doctest.h>
#include <neutrino/video/sprite/bitmask.hh>

#include <limits>

TEST_SUITE("neutrino::video::sprite::bitmask") {
    TEST_CASE("Constructor and dimensions") {
        neutrino::bitmask empty_mask;
        CHECK(empty_mask.width() == 0);
        CHECK(empty_mask.height() == 0);

        neutrino::bitmask mask(8, 12);
        CHECK(mask.width() == 8);
        CHECK(mask.height() == 12);

        // All bits should be initialized to false
        for (int y = 0; y < 12; ++y) {
            for (int x = 0; x < 8; ++x) {
                CHECK_FALSE(mask.get(x, y));
            }
        }
    }

    TEST_CASE("Get and Set values inside and outside bounds") {
        neutrino::bitmask mask(4, 4);

        mask.set(1, 2, true);
        CHECK(mask.get(1, 2));

        mask.set(1, 2, false);
        CHECK_FALSE(mask.get(1, 2));

        // Corner boundaries
        mask.set(0, 0, true);
        mask.set(3, 3, true);
        CHECK(mask.get(0, 0));
        CHECK(mask.get(3, 3));

        // Negative boundaries (no-op sets, should safely return false)
        mask.set(-1, 0, true);
        mask.set(0, -1, true);
        CHECK_FALSE(mask.get(-1, 0));
        CHECK_FALSE(mask.get(0, -1));

        // Positive out-of-bounds (no-op sets, should safely return false)
        mask.set(4, 0, true);
        mask.set(0, 4, true);
        mask.set(4, 4, true);
        CHECK_FALSE(mask.get(4, 0));
        CHECK_FALSE(mask.get(0, 4));
        CHECK_FALSE(mask.get(4, 4));
    }

    TEST_CASE("Constructor rejects invalid dimensions") {
        CHECK_THROWS(neutrino::bitmask(-1, 5));
        CHECK_THROWS(neutrino::bitmask(5, -1));
        CHECK_THROWS(neutrino::bitmask(65536, 65536));
        CHECK_THROWS(neutrino::bitmask(std::numeric_limits <int>::max(), 2));
    }

    TEST_CASE("Overlaps: No-overlap scenarios (Empty, Transparent, Disjoint)") {
        neutrino::bitmask emptyA;
        neutrino::bitmask emptyB;
        CHECK_FALSE(emptyA.overlaps(emptyB, 0, 0));

        neutrino::bitmask maskA(4, 4);
        neutrino::bitmask maskB(4, 4);
        // Completely transparent masks with valid dimensions should not overlap
        CHECK_FALSE(maskA.overlaps(maskB, 0, 0));

        // Populated masks with disjoint bounds (extremely far translations)
        maskA.set(0, 0, true);
        maskB.set(0, 0, true);
        CHECK_FALSE(maskA.overlaps(maskB, 100, 100));
        CHECK_FALSE(maskA.overlaps(maskB, -100, -100));
        CHECK_FALSE(maskA.overlaps(maskB, 4, 0));  // Exactly touching on right side
        CHECK_FALSE(maskA.overlaps(maskB, -4, 0)); // Exactly touching on left side
        CHECK_FALSE(maskA.overlaps(maskB, 0, 4));  // Exactly touching on bottom side
        CHECK_FALSE(maskA.overlaps(maskB, 0, -4)); // Exactly touching on top side
    }

    TEST_CASE("Overlaps: Relative Quadrant Offset Translations") {
        // We test all 4 quadrants of translations to make sure
        // loops work correctly with relative dx and dy offsets.
        neutrino::bitmask mask1(3, 3);
        neutrino::bitmask mask2(3, 3);

        mask1.set(1, 1, true); // Solid pixel in the center
        mask2.set(1, 1, true); // Solid pixel in the center

        // 1. Center / No Offset (dx = 0, dy = 0)
        CHECK(mask1.overlaps(mask2, 0, 0));

        // 2. Shift Top-Left (dx < 0, dy < 0)
        // Shifting mask2 top-left means its center shifts to (0,0) in mask1's space
        CHECK_FALSE(mask1.overlaps(mask2, -1, -1));
        mask1.set(0, 0, true);
        CHECK(mask1.overlaps(mask2, -1, -1));

        // 3. Shift Top-Right (dx > 0, dy < 0)
        // Shifting mask2 top-right means its center shifts to (2,0) in mask1's space
        CHECK_FALSE(mask1.overlaps(mask2, 1, -1));
        mask1.set(2, 0, true);
        CHECK(mask1.overlaps(mask2, 1, -1));

        // 4. Shift Bottom-Left (dx < 0, dy > 0)
        // Shifting mask2 bottom-left means its center shifts to (0,2) in mask1's space
        CHECK_FALSE(mask1.overlaps(mask2, -1, 1));
        mask1.set(0, 2, true);
        CHECK(mask1.overlaps(mask2, -1, 1));

        // 5. Shift Bottom-Right (dx > 0, dy > 0)
        // Shifting mask2 bottom-right means its center shifts to (2,2) in mask1's space
        CHECK_FALSE(mask1.overlaps(mask2, 1, 1));
        mask1.set(2, 2, true);
        CHECK(mask1.overlaps(mask2, 1, 1));
    }

    TEST_CASE("Overlaps: Solid vs Transparent Boundary Intersections") {
        neutrino::bitmask mask1(2, 2);
        neutrino::bitmask mask2(2, 2);

        // Solid diagonal in mask1
        mask1.set(0, 0, true);
        mask1.set(1, 1, true);

        // Solid diagonal in mask2
        mask2.set(0, 1, true);
        mask2.set(1, 0, true);

        // With offset (0,0), their diagonals cross but do they overlap?
        // Digonals cross at the sub-pixel boundary, but at integer level:
        // mask1: (0,0)=true, (0,1)=false, (1,0)=false, (1,1)=true
        // mask2: (0,0)=false, (0,1)=true, (1,0)=true, (1,1)=false
        // They should NOT overlap since no cell has true in both masks.
        CHECK_FALSE(mask1.overlaps(mask2, 0, 0));

        // Shifting mask2 by dx = 1 makes:
        // mask2 shifted: (1,1)=true (mapped from (0,1)), (1,0)=false
        // Now it overlaps with mask1's (1,1)
        CHECK(mask1.overlaps(mask2, 1, 0));
    }

    TEST_CASE("Overlaps: Self Collision") {
        neutrino::bitmask mask(4, 4);
        CHECK_FALSE(mask.overlaps(mask, 0, 0)); // Transparent self collision

        mask.set(2, 2, true);
        CHECK(mask.overlaps(mask, 0, 0)); // Solid self collision

        // Shifts that keep the pixel inside its own boundary
        CHECK_FALSE(mask.overlaps(mask, 1, 1));
        mask.set(3, 3, true);
        CHECK(mask.overlaps(mask, 1, 1)); // (2,2) shifted by (1,1) hits (3,3)
    }

    TEST_CASE("Overlaps: Multi-pixel scanning / Large masks") {
        // Creates two 64x64 masks to test SIMD / vector alignment behavior
        neutrino::bitmask mask1(64, 64);
        neutrino::bitmask mask2(64, 64);

        // Set one pixel at the very end of row
        mask1.set(63, 63, true);
        mask2.set(0, 0, true);

        // Offsets needed to make them collide
        CHECK_FALSE(mask1.overlaps(mask2, 0, 0));
        CHECK(mask1.overlaps(mask2, 63, 63));
    }
}
