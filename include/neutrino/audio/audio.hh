/// @file audio.hh
/// @brief Global audio functions.
///
/// The application is effectively a singleton, so the audio subsystem is
/// exposed as free functions. The underlying system is owned by
/// neutrino::application and is fail-soft: if no audio device can be opened,
/// audio_active() returns false and every call below becomes a silent no-op.
///
/// Volume model: effective stream volume = master × group × caller, where
/// group is the sfx or music volume. Group and master changes are applied
/// immediately to live streams.

#pragma once

#include <chrono>
#include <functional>
#include <iosfwd>
#include <memory>
#include <string>

#include <neutrino/neutrino_export.h>
#include <neutrino/audio/sound_effect.hh>
#include <neutrino/audio/music_stream.hh>
#include <neutrino/audio/pc_speaker.hh>

namespace musac {
    class decoder;
    class io_stream;
}

namespace neutrino {
    /// @brief True if an audio device is open and sound can actually play.
    NEUTRINO_EXPORT bool audio_active();

    // Mixer. Volumes are clamped to [0, 1].
    NEUTRINO_EXPORT void set_master_volume(float volume);
    NEUTRINO_EXPORT float get_master_volume();
    NEUTRINO_EXPORT void set_sfx_volume(float volume);
    NEUTRINO_EXPORT float get_sfx_volume();
    NEUTRINO_EXPORT void set_music_volume(float volume);
    NEUTRINO_EXPORT float get_music_volume();

    // Assets. Throws if the file cannot be opened; returns inert objects
    // when audio is inactive.
    NEUTRINO_EXPORT sound_effect load_sfx(const std::string& path);
    NEUTRINO_EXPORT music_stream load_music(const std::string& path);
    NEUTRINO_EXPORT pc_speaker create_pc_speaker();

    // Stream-based loading (archives, embedded resources, network, ...).
    // The istream is read to EOF during the call and may be destroyed
    // afterwards; the decoded-from bytes are kept in memory by the returned
    // object. Throws if nothing can be read.
    NEUTRINO_EXPORT sound_effect load_sfx(std::istream& is);
    NEUTRINO_EXPORT music_stream load_music(std::istream& is);

    // Music slot: the single background-music channel every game has.
    // play_music replaces whatever is playing (the old track fades out with
    // the same fade_time). For the rare multi-stream case use load_music().
    NEUTRINO_EXPORT void play_music(const std::string& path, bool loop = true,
                                    std::chrono::microseconds fade_time = {});
    NEUTRINO_EXPORT void play_music(std::istream& is, bool loop = true,
                                    std::chrono::microseconds fade_time = {});
    NEUTRINO_EXPORT void stop_music(std::chrono::microseconds fade_time = {});
    NEUTRINO_EXPORT void pause_music(std::chrono::microseconds fade_time = {});
    NEUTRINO_EXPORT void resume_music(std::chrono::microseconds fade_time = {});
    NEUTRINO_EXPORT bool is_music_playing();

    /// @brief Register a game-specific custom decoder/codec.
    NEUTRINO_EXPORT void register_decoder(
        const std::function<bool(musac::io_stream*)>& accept_func,
        const std::function<std::unique_ptr<musac::decoder>()>& factory_func,
        int priority = 0
    );
}
