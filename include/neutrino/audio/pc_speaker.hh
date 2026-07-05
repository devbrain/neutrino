//
// Created by igor on 02/07/2026.
//

#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <neutrino/neutrino_export.h>

namespace musac {
    class pc_speaker_stream;
}

namespace neutrino {
    class NEUTRINO_EXPORT pc_speaker {
    public:
        pc_speaker();
        explicit pc_speaker(std::unique_ptr<musac::pc_speaker_stream> stream);
        ~pc_speaker();

        pc_speaker(pc_speaker&&) noexcept;
        pc_speaker& operator=(pc_speaker&&) noexcept;

        pc_speaker(const pc_speaker&) = delete;
        pc_speaker& operator=(const pc_speaker&) = delete;

        // Play the speaker queue.
        void play();

        // Pause playback.
        void pause();

        // Stop playback and clear the queue.
        void stop();

        // Queue a tone.
        void sound(float frequency_hz, std::chrono::milliseconds duration);

        // Play a quick beep (convenience method).
        void beep(float frequency_hz = 1000.0f);

        // Queue a silence period.
        void silence(std::chrono::milliseconds duration);

        // Clear all pending commands in the queue.
        void clear_queue();

        // Play an MML sequence.
        bool play_mml(const std::string& mml, bool strict = false);

        // Queue queries.
        [[nodiscard]] bool is_queue_empty() const;
        [[nodiscard]] size_t queue_size() const;

    private:
        std::unique_ptr<musac::pc_speaker_stream> m_stream;
    };
}
