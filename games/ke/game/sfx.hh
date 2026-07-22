//
// KE sound-effect player: loads KE_MAIN.DIG once (via the KE dig_decoder) into one
// overlapping sound_effect per sample, and plays a ke_sfx event by index (tab.md §8).
//

#pragma once

#include <ke/assets/ke_sfx.hh>

namespace rs {
    struct game_resources;
}

namespace ke {

    class audio {
        public:
            static audio& instance();

            // Register the DIG decoder and build one sound_effect per KE_MAIN.DIG sample.
            // Fail-soft: a no-op if audio is inactive or the bank is missing.
            void load(const rs::game_resources& res);

            // Play @p e: pick a sample from its range (rotating within a variation pool).
            void play(rs::ke_sfx e);

        private:
            audio() = default;
    };

} // namespace ke
