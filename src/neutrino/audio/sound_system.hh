/// @file sound_system.hh
/// @brief Internal audio service: owns the backend, codec registry and device.
///
/// One instance is owned by application and published through
/// service_locator (as a pointer — it is absent before the application
/// exists and after it is destroyed). Games never see this class; they use
/// the free functions in <neutrino/audio/audio.hh>.

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

namespace musac {
    class audio_backend;
    class audio_device;
    class audio_stream;
    class decoders_registry;
    class io_stream;
}

namespace neutrino {
    class sound_effect;
    class music_stream;

    /// Fail-soft: if the backend or device cannot be initialized, the
    /// instance stays constructible with active() == false and every
    /// playback call is a silent no-op.
    class sound_system {
        public:
            sound_system();
            ~sound_system();

            sound_system(const sound_system&) = delete;
            sound_system& operator=(const sound_system&) = delete;

            [[nodiscard]] bool active() const;
            [[nodiscard]] musac::audio_device* device() const;
            [[nodiscard]] std::shared_ptr <musac::decoders_registry> registry() const;

            // Mixer. Master drives the device gain; group changes are
            // re-applied to every live stream (group × caller).
            void set_master_volume(float volume);
            [[nodiscard]] float master_volume() const;
            void set_sfx_volume(float volume);
            [[nodiscard]] float sfx_volume() const;
            void set_music_volume(float volume);
            [[nodiscard]] float music_volume() const;

            // Music slot — single background-music channel, replace-on-play.
            void play_music(const std::string& path, bool loop, std::chrono::microseconds fade_time);
            void play_music(std::unique_ptr <musac::io_stream> io, bool loop, std::chrono::microseconds fade_time);
            void stop_music(std::chrono::microseconds fade_time);
            void pause_music(std::chrono::microseconds fade_time);
            void resume_music(std::chrono::microseconds fade_time);
            [[nodiscard]] bool music_playing() const;

            // Live-volume registry: sound_effect / music_stream instances
            // register themselves so group volume changes reach their
            // streams. Registration is idempotent.
            void register_effect(sound_effect* effect);
            void unregister_effect(sound_effect* effect);
            void register_music(music_stream* music);
            void unregister_music(music_stream* music);

        private:
            std::shared_ptr <musac::audio_backend> m_backend;
            std::shared_ptr <musac::decoders_registry> m_codecs;
            std::unique_ptr <musac::audio_device> m_device;
            bool m_system_up = false;

            float m_master_volume = 1.0f;
            float m_sfx_volume = 1.0f;
            float m_music_volume = 1.0f;

            std::unique_ptr <musac::audio_stream> m_music_slot;
            // Previous slot occupant, kept alive so its fade-out is audible.
            std::unique_ptr <musac::audio_stream> m_retiring_music;

            std::vector <sound_effect*> m_effects;
            std::vector <music_stream*> m_musics;
    };
}
