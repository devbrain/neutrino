//
// Created by igor on 02/07/2026.
//

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <chrono>
#include <string>
#include <functional>
#include <neutrino/neutrino_export.h>

namespace musac {
    class audio_source;
    class audio_stream;
}

namespace neutrino {
    class sound_system;
    namespace audio_detail { struct callback_relay; }

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
        /// istream). The first play() decodes + resamples it to device-rate PCM once and
        /// caches that, so that play and every later one (and overlapping channels) need no
        /// decode or resampler; an effect that is never played is never resampled. Falls
        /// back to per-channel streaming if the sample can't be pre-rendered (unknown
        /// format, or too long to be a short effect).
        explicit sound_effect(std::shared_ptr<const std::vector<uint8_t>> data);
        /// @brief Effect backed by pre-decoded, device-rate PCM (interleaved float),
        /// as produced by load_sfx once at load. Channels play it back with no decode
        /// and no resampler. @p rate must equal the output device rate and @p channels
        /// its interleave; the buffer is shared read-only across concurrent channels.
        sound_effect(std::shared_ptr<const std::vector<float>> pcm, unsigned rate, unsigned channels);
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

        /// @brief Set a callback invoked when a channel of this effect finishes playing.
        /// It runs on the main thread during the app's per-frame update -- not on the audio
        /// thread -- so it may freely touch game state, at up to one frame of latency. Because
        /// an effect can sound several overlapping channels, it fires once per channel that
        /// ends. stop() does not trigger it. Replaces any previous callback; pass {} to clear.
        void on_finished(std::function<void()> cb);

    private:
        friend class sound_system;
        /// @brief Re-apply the sfx group volume as (caller volume × @p group) to
        /// all live channels; invoked by sound_system when the group volume changes.
        void apply_group_volume(float group);
        /// @brief Wire @p stream's finish callback to this effect's relay (audio-thread safe).
        void arm_channel(musac::audio_stream& stream);
        /// @brief Drain relayed finish events and invoke the callback; called on the main
        /// thread by sound_system once per frame.
        void dispatch_pending();

        struct channel {
            std::unique_ptr<musac::audio_stream> stream;
            float caller_volume = 1.0f;
        };

        std::string m_path;
        std::shared_ptr<const std::vector<uint8_t>> m_data;
        std::shared_ptr<const std::vector<float>> m_pcm; // device-rate PCM (interleaved): pcm ctor or first-play capture
        unsigned m_pcm_rate = 0;
        unsigned m_pcm_channels = 0;
        bool m_pcm_tried = false; // has the first-play lazy capture of m_data been attempted?
        std::shared_ptr<musac::audio_source> m_source;
        std::vector<channel> m_channels;
        std::shared_ptr<audio_detail::callback_relay> m_relay; // audio-thread finish counters
        std::function<void()> m_on_finished;
    };
}
