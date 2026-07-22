//
// See sfx.hh.
//

#include <ke/game/sfx.hh>

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include <failsafe/logger.hh>

#include <musac/sdk/io_stream.hh>

#include <neutrino/audio/audio.hh>

#include <ke/format/dig.hh>
#include <ke/resources/resources.hh>

namespace ke {

    namespace {
        // The built SFX: one overlapping sound_effect per KE_MAIN.DIG sample, plus a
        // rotating cursor so a variation pool picks a different member each time.
        std::vector<neutrino::sound_effect> g_effects;
        unsigned g_rot = 0;
    }

    audio& audio::instance() {
        static audio a;
        return a;
    }

    void audio::load(const rs::game_resources& res) {
        g_effects.clear();
        if (!neutrino::audio_active()) {
            return;
        }

        // Let neutrino's audio decode DIG streams (used per channel by load_sfx below).
        neutrino::register_decoder(
            rs::dig_decoder::accept,
            [] { return std::make_unique<rs::dig_decoder>(); },
            100);

        const auto it = res.audio.find(rs::ke_sfx_bank);
        if (it == res.audio.end()) {
            LOG_WARN("ke: no", rs::ke_sfx_bank, "-- SFX disabled");
            return;
        }
        const std::vector<std::uint8_t>& bank = it->second;

        rs::dig_decoder dec;
        auto stream = musac::io_from_memory(bank.data(), bank.size());
        try {
            dec.open(stream.get());
        } catch (const std::exception& e) {
            LOG_ERROR("ke: failed to open", rs::ke_sfx_bank, ":", e.what());
            return;
        }

        // One overlapping sound_effect per sample: a single-sample DIG blob decoded by the
        // registered dig_decoder. load_sfx keeps the bytes and decodes a fresh view per play.
        g_effects.reserve(dec.sample_count());
        for (std::uint16_t i = 0; i < dec.sample_count(); ++i) {
            const std::vector<std::uint8_t> one = dec.extract_sample(i);
            std::string bytes(reinterpret_cast<const char*>(one.data()), one.size());
            std::istringstream is(std::move(bytes), std::ios::binary);
            g_effects.push_back(neutrino::load_sfx(is));
        }
        LOG_DEBUG("ke: loaded", g_effects.size(), "SFX from", rs::ke_sfx_bank);
    }

    void audio::play(rs::ke_sfx e) {
        const rs::ke_sfx_range r = rs::ke_sfx_samples(e);
        if (r.count == 0) {
            return;
        }
        const std::uint16_t idx = r.count == 1 ? r.first
                                               : static_cast<std::uint16_t>(r.first + (g_rot++ % r.count));
        if (idx < g_effects.size()) {
            g_effects[idx].play();
        }
    }

} // namespace ke
