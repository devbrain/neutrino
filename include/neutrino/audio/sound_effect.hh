//
// Created by igor on 02/07/2026.
//

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <chrono>
#include <string>
#include <neutrino/neutrino_export.h>

namespace musac {
    class audio_source;
    class audio_stream;
}

namespace neutrino {
    class sound_system;

    /// @brief A short, replayable sound effect that can sound several overlapping
    /// instances at once, returned by load_sfx().
    ///
    /// Each play() uses (or spawns) a channel, so the same effect can fire many
    /// times concurrently. Move-only, non-copyable. A default-constructed
    /// (inert) effect treats play() as a silent no-op. Effective per-channel
    /// volume is the play() volume times the sfx group volume.
    class NEUTRINO_EXPORT sound_effect {
    public:
        /// @brief Construct an inert effect (no backing audio; play() is a no-op).
        sound_effect();
        /// @brief Effect that (re-)opens @p path from disk on demand: a fresh
        /// file handle is decoded per concurrent channel, so the same file can
        /// play any number of overlapping instances.
        explicit sound_effect(std::string path);
        /// @brief Effect backed by a pre-decoded audio source. Because the source
        /// is move-only it drives a single channel, so replays restart it rather
        /// than overlapping.
        explicit sound_effect(std::shared_ptr<musac::audio_source> source);
        /// @brief Effect backed by an in-memory encoded file (e.g. slurped from an
        /// istream); each concurrent channel decodes its own view of the data.
        explicit sound_effect(std::shared_ptr<const std::vector<uint8_t>> data);
        ~sound_effect();

        sound_effect(sound_effect&&) noexcept;
        sound_effect& operator=(sound_effect&&) noexcept;

        sound_effect(const sound_effect&) = delete;
        sound_effect& operator=(const sound_effect&) = delete;

        /// @brief Play the effect at @p volume (times the sfx group volume), fading
        /// in over @p fade_time. Reuses an idle channel or spawns a new one, so
        /// calling again while it is still sounding overlaps a fresh instance
        /// (except for the single-channel audio_source variant). No-op if audio is inactive.
        void play(float volume = 1.0f, std::chrono::microseconds fade_time = {});

        /// @brief Stop every active channel of this effect, fading out over @p fade_time.
        void stop(std::chrono::microseconds fade_time = {});

        /// @brief True if any channel of this effect is currently playing.
        [[nodiscard]] bool is_playing() const;

    private:
        friend class sound_system;
        /// @brief Re-apply the sfx group volume as (caller volume × @p group) to
        /// all live channels; invoked by sound_system when the group volume changes.
        void apply_group_volume(float group);

        struct channel {
            std::unique_ptr<musac::audio_stream> stream;
            float caller_volume = 1.0f;
        };

        std::string m_path;
        std::shared_ptr<const std::vector<uint8_t>> m_data;
        std::shared_ptr<musac::audio_source> m_source;
        std::vector<channel> m_channels;
    };
}
