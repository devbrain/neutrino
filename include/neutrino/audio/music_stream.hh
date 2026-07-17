//
// Created by igor on 02/07/2026.
//

#pragma once

#include <memory>
#include <chrono>
#include <neutrino/neutrino_export.h>

namespace musac {
    class audio_stream;
}

namespace neutrino {
    class sound_system;

    /// @brief A standalone, independently-controllable music track, returned by load_music().
    ///
    /// Unlike the shared music slot driven by neutrino::play_music(), a
    /// music_stream is owned by the caller and lets several tracks play at once
    /// (e.g. layered/adaptive music). Move-only, non-copyable. A
    /// default-constructed (inert) stream, as returned when audio is inactive,
    /// treats every call as a silent no-op. Its effective volume is the
    /// per-stream factor times the music group volume.
    class NEUTRINO_EXPORT music_stream {
    public:
        /// @brief Construct an inert stream (no backing audio; all calls are no-ops).
        music_stream();
        /// @brief Wrap an opened backing stream; used internally by load_music().
        explicit music_stream(std::unique_ptr<musac::audio_stream> stream);
        ~music_stream();

        music_stream(music_stream&&) noexcept;
        music_stream& operator=(music_stream&&) noexcept;

        music_stream(const music_stream&) = delete;
        music_stream& operator=(const music_stream&) = delete;

        /// @brief Start playing from the current position, fading in over @p fade_time.
        /// @param loop when true (default) the track repeats indefinitely.
        void play(bool loop = true, std::chrono::microseconds fade_time = {});

        /// @brief Pause playback, fading out over @p fade_time; resume() continues from here.
        void pause(std::chrono::microseconds fade_time = {});

        /// @brief Resume paused playback, fading in over @p fade_time.
        void resume(std::chrono::microseconds fade_time = {});

        /// @brief Stop playback, fading out over @p fade_time.
        void stop(std::chrono::microseconds fade_time = {});

        /// @brief Set this stream's own volume factor, clamped to [0, 1].
        ///
        /// This is a per-stream multiplier, not the absolute output level: the
        /// effective volume is this factor times the current music group volume
        /// (see neutrino::set_music_volume). A value of 1.0 therefore plays at
        /// full group volume, not necessarily full system volume.
        void set_volume(float volume);

        /// @brief True while the track is actively playing (false if paused, stopped, or inert).
        [[nodiscard]] bool is_playing() const;

        /// @brief True while the track is paused (false if playing, stopped, or inert).
        [[nodiscard]] bool is_paused() const;

        /// @brief Reset playback to the start of the track.
        void rewind();

    private:
        friend class sound_system;
        /// @brief Re-apply the music group volume as (caller factor × @p group);
        /// invoked by sound_system when the group volume changes.
        void apply_group_volume(float group);

        std::unique_ptr<musac::audio_stream> m_stream;
        float m_caller_volume = 1.0f;
    };
}
