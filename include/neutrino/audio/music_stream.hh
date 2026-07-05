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

    class NEUTRINO_EXPORT music_stream {
    public:
        music_stream();
        explicit music_stream(std::unique_ptr<musac::audio_stream> stream);
        ~music_stream();

        music_stream(music_stream&&) noexcept;
        music_stream& operator=(music_stream&&) noexcept;

        music_stream(const music_stream&) = delete;
        music_stream& operator=(const music_stream&) = delete;

        // Play the music stream.
        void play(bool loop = true, std::chrono::microseconds fade_time = {});

        // Pause playback.
        void pause(std::chrono::microseconds fade_time = {});

        // Resume playback.
        void resume(std::chrono::microseconds fade_time = {});

        // Stop playback.
        void stop(std::chrono::microseconds fade_time = {});

        // Set the volume multiplier (0.0 to 1.0)
        void set_volume(float volume);

        // Check if the music stream is playing.
        [[nodiscard]] bool is_playing() const;

        // Check if the music stream is paused.
        [[nodiscard]] bool is_paused() const;

        // Rewind the stream to the beginning.
        void rewind();

    private:
        friend class sound_system;
        // Re-apply the music group volume (group × caller).
        void apply_group_volume(float group);

        std::unique_ptr<musac::audio_stream> m_stream;
        float m_caller_volume = 1.0f;
    };
}
