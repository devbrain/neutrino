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

    class NEUTRINO_EXPORT sound_effect {
    public:
        sound_effect();
        explicit sound_effect(std::string path);
        explicit sound_effect(std::shared_ptr<musac::audio_source> source);
        // Effect backed by an in-memory encoded file (e.g. slurped from an
        // istream); each concurrent channel decodes its own view of the data.
        explicit sound_effect(std::shared_ptr<const std::vector<uint8_t>> data);
        ~sound_effect();

        sound_effect(sound_effect&&) noexcept;
        sound_effect& operator=(sound_effect&&) noexcept;

        sound_effect(const sound_effect&) = delete;
        sound_effect& operator=(const sound_effect&) = delete;

        // Play the sound effect. If already playing, it will spawn a concurrent channel.
        void play(float volume = 1.0f, std::chrono::microseconds fade_time = {});

        // Stop all active channels playing this sound effect.
        void stop(std::chrono::microseconds fade_time = {});

        // Check if any channel is currently playing this sound effect.
        [[nodiscard]] bool is_playing() const;

    private:
        friend class sound_system;
        // Re-apply the sfx group volume to live channels (group × caller).
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
