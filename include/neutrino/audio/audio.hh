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

    /// @brief Set the master volume; clamped to [0, 1] and applied immediately to all live audio. No-op if audio is inactive.
    NEUTRINO_EXPORT void set_master_volume(float volume);
    /// @brief Current master volume in [0, 1] (1.0 if audio is inactive).
    NEUTRINO_EXPORT float get_master_volume();
    /// @brief Set the sound-effects group volume; clamped to [0, 1] and applied immediately to live effects. No-op if audio is inactive.
    NEUTRINO_EXPORT void set_sfx_volume(float volume);
    /// @brief Current sfx group volume in [0, 1] (1.0 if audio is inactive).
    NEUTRINO_EXPORT float get_sfx_volume();
    /// @brief Set the music group volume; clamped to [0, 1] and applied immediately to the music slot and all live music streams. No-op if audio is inactive.
    NEUTRINO_EXPORT void set_music_volume(float volume);
    /// @brief Current music group volume in [0, 1] (1.0 if audio is inactive).
    NEUTRINO_EXPORT float get_music_volume();

    /// @brief Load a sound effect from a file. The path is validated eagerly (a
    /// fresh file handle is decoded per channel at play time).
    /// @throws std::runtime_error if @p path cannot be opened.
    NEUTRINO_EXPORT sound_effect load_sfx(const std::string& path);
    /// @brief Load a music stream from a file.
    /// @throws std::runtime_error if @p path cannot be opened. Returns an inert
    /// (silent) stream when audio is inactive.
    NEUTRINO_EXPORT music_stream load_music(const std::string& path);
    /// @brief Create a PC-speaker tone generator. Returns an inert speaker when
    /// audio is inactive; never throws.
    NEUTRINO_EXPORT pc_speaker create_pc_speaker();

    /// @brief Load a sound effect from a stream (archives, embedded resources,
    /// network, ...). The istream is read to EOF during the call and may be
    /// destroyed afterwards; the encoded bytes are kept in memory by the
    /// returned object and each concurrent channel decodes its own view.
    /// @throws std::runtime_error if nothing can be read from @p is.
    NEUTRINO_EXPORT sound_effect load_sfx(std::istream& is);
    /// @brief Load a music stream from a stream. The istream is read to EOF
    /// during the call and may be destroyed afterwards; the encoded bytes are
    /// kept in memory by the returned object.
    /// @throws std::runtime_error if nothing can be read from @p is. Returns an
    /// inert (silent) stream when audio is inactive.
    NEUTRINO_EXPORT music_stream load_music(std::istream& is);

    /// @brief Play a track on the shared background-music slot, replacing whatever
    /// is currently playing. The outgoing track fades out over @p fade_time (and
    /// the new one fades in) so the transition is audible.
    /// @param loop when true (default) the track repeats indefinitely.
    /// @throws std::runtime_error if @p path cannot be opened. No-op if audio is
    /// inactive. For the rare multi-stream case use load_music() instead.
    NEUTRINO_EXPORT void play_music(const std::string& path, bool loop = true,
                                    std::chrono::microseconds fade_time = {});
    /// @brief Play a track on the shared music slot from a stream (read to EOF
    /// during the call; the bytes are copied in). Replaces the current track,
    /// crossfading over @p fade_time.
    /// @throws std::runtime_error if nothing can be read from @p is. No-op if
    /// audio is inactive.
    NEUTRINO_EXPORT void play_music(std::istream& is, bool loop = true,
                                    std::chrono::microseconds fade_time = {});
    /// @brief Stop the music slot, fading out over @p fade_time. No-op if nothing is playing.
    NEUTRINO_EXPORT void stop_music(std::chrono::microseconds fade_time = {});
    /// @brief Pause the music slot, fading out over @p fade_time; resume_music() continues from here.
    NEUTRINO_EXPORT void pause_music(std::chrono::microseconds fade_time = {});
    /// @brief Resume the paused music slot, fading in over @p fade_time.
    NEUTRINO_EXPORT void resume_music(std::chrono::microseconds fade_time = {});
    /// @brief True if the music slot currently holds a playing (non-paused) track.
    NEUTRINO_EXPORT bool is_music_playing();

    /// @brief Register a game-specific custom decoder/codec.
    NEUTRINO_EXPORT void register_decoder(
        const std::function<bool(musac::io_stream*)>& accept_func,
        const std::function<std::unique_ptr<musac::decoder>()>& factory_func,
        int priority = 0
    );
}
