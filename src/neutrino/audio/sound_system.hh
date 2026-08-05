/// @file sound_system.hh
/// @brief Internal audio service: owns the backend, codec registry and device.
///
/// One instance is owned by application and published through
/// service_locator (as a pointer — it is absent before the application
/// exists and after it is destroyed). Games never see this class; they use
/// the free functions in <neutrino/audio/audio.hh>.

#pragma once

#include <chrono>
#include <functional>
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
    namespace audio_detail { struct callback_relay; }

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

            // Music-slot callbacks: invoked on the main thread (see dispatch_callbacks) when
            // the current slot track finishes on its own / wraps a loop. Pass {} to clear.
            void set_music_finished_callback(std::function<void()> cb);
            void set_music_looped_callback(std::function<void()> cb);

            // Main-thread pump: drain the relayed audio-thread finish/loop events of every
            // registered effect / music_stream and the music slot, invoking their callbacks.
            // Called once per frame by application::on_update.
            void dispatch_callbacks();

            // Live-volume registry: sound_effect / music_stream instances
            // register themselves so group volume changes reach their
            // streams. Registration is idempotent.
            void register_effect(sound_effect* effect);
            void unregister_effect(sound_effect* effect);
            void register_music(music_stream* music);
            void unregister_music(music_stream* music);

            // Codec ids already handed to the musac registry, for register_decoder_once().
            // musac's registry appends unconditionally and offers no unregister, so
            // de-duplication has to happen on this side. Returns true if @p codec_id was new
            // (the caller should register); false if it is already present.
            [[nodiscard]] bool claim_codec_id(const std::string& codec_id);

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
            std::vector <std::string>   m_codec_ids; // see claim_codec_id

            // Finish/loop relay + consumer callbacks for the music slot (m_music_slot).
            std::shared_ptr <audio_detail::callback_relay> m_slot_relay;
            std::function <void()> m_on_music_finished;
            std::function <void()> m_on_music_looped;
    };
}
