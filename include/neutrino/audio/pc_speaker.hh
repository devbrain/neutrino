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
    /// @brief Retro PC-speaker style tone generator, obtained from create_pc_speaker().
    ///
    /// Tones, beeps and silences are appended to an internal queue and played
    /// back in order. Move-only, non-copyable. A default-constructed (inert)
    /// speaker, as returned when audio is inactive, accepts every call as a
    /// silent no-op. Its volume follows the sfx group volume.
    class NEUTRINO_EXPORT pc_speaker {
    public:
        /// @brief Construct an inert speaker (no backing stream; all calls are no-ops).
        pc_speaker();
        /// @brief Wrap an opened backing stream; used internally by create_pc_speaker().
        explicit pc_speaker(std::unique_ptr<musac::pc_speaker_stream> stream);
        ~pc_speaker();

        pc_speaker(pc_speaker&&) noexcept;
        pc_speaker& operator=(pc_speaker&&) noexcept;

        pc_speaker(const pc_speaker&) = delete;
        pc_speaker& operator=(const pc_speaker&) = delete;

        /// @brief Start (or resume) playing the queued tones, applying the current sfx group volume.
        void play();

        /// @brief Pause playback; the queue and playback position are preserved for the next play().
        void pause();

        /// @brief Stop playback and discard the queue.
        void stop();

        /// @brief Append a tone of @p frequency_hz lasting @p duration to the queue.
        void sound(float frequency_hz, std::chrono::milliseconds duration);

        /// @brief Convenience: queue a short fixed-length beep at @p frequency_hz.
        void beep(float frequency_hz = 1000.0f);

        /// @brief Append a silent gap of @p duration to the queue.
        void silence(std::chrono::milliseconds duration);

        /// @brief Discard all pending queued commands without stopping current playback.
        void clear_queue();

        /// @brief Parse an MML (Music Macro Language) string and append the
        /// resulting tones and rests to the queue.
        /// @param strict when true, any syntax error aborts parsing and the call
        /// fails; when false (default), the parser is lenient and skips over
        /// malformed tokens.
        /// @return true if parsing succeeded and commands were queued; false on a
        /// parse failure (in strict mode) or on an inert speaker.
        bool play_mml(const std::string& mml, bool strict = false);

        /// @brief True if no commands are pending (also true for an inert speaker).
        [[nodiscard]] bool is_queue_empty() const;
        /// @brief Number of commands currently pending in the queue (0 for an inert speaker).
        [[nodiscard]] size_t queue_size() const;

    private:
        std::unique_ptr<musac::pc_speaker_stream> m_stream;
    };
}
