//
// Created by igor on 02/07/2026.
//

#include <neutrino/audio/music_stream.hh>
#include "sound_system.hh"
#include "services/service_access.hh"
#include <musac/stream.hh>

namespace neutrino {

    static float music_group_volume() {
        auto* ss = maybe_sound_system();
        return ss ? ss->music_volume() : 1.0f;
    }

    music_stream::music_stream()
        : m_stream(nullptr) {
    }

    music_stream::music_stream(std::unique_ptr<musac::audio_stream> stream)
        : m_stream(std::move(stream)) {
        if (m_stream) {
            m_stream->open();
            if (auto* ss = maybe_sound_system()) {
                ss->register_music(this);
            }
        }
    }

    music_stream::~music_stream() {
        if (auto* ss = maybe_sound_system()) {
            ss->unregister_music(this);
        }
        m_stream.reset();
    }

    music_stream::music_stream(music_stream&& other) noexcept
        : m_stream(std::move(other.m_stream)),
          m_caller_volume(other.m_caller_volume) {
        // other stays registered until its destructor runs; its stream is
        // null now, so that registration is harmless.
        if (m_stream) {
            if (auto* ss = maybe_sound_system()) {
                ss->register_music(this);
            }
        }
    }

    music_stream& music_stream::operator=(music_stream&& other) noexcept {
        // Registration is *conditional* on holding a stream, so it must track the stream
        // we replace: unregister ourselves, move, then re-register only if we now hold a
        // stream. A defaulted move-assign would leave a formerly-null (unregistered) target
        // holding a live stream but unregistered -- it would miss group-volume and shutdown.
        // The source keeps its registration until its destructor (its stream is null now,
        // so that registration is harmless, matching the move constructor).
        if (this != &other) {
            if (auto* ss = maybe_sound_system()) {
                ss->unregister_music(this);
            }
            m_stream = std::move(other.m_stream);
            m_caller_volume = other.m_caller_volume;
            if (m_stream) {
                if (auto* ss = maybe_sound_system()) {
                    ss->register_music(this);
                }
            }
        }
        return *this;
    }

    void music_stream::apply_group_volume(float group) {
        if (m_stream) {
            m_stream->set_volume(m_caller_volume * group);
        }
    }

    void music_stream::play(bool loop, std::chrono::microseconds fade_time) {
        if (m_stream) {
            // Apply volume configuration before playing
            m_stream->set_volume(m_caller_volume * music_group_volume());
            m_stream->play(loop ? 0 : 1, fade_time);
        }
    }

    void music_stream::pause(std::chrono::microseconds fade_time) {
        if (m_stream) {
            m_stream->pause(fade_time);
        }
    }

    void music_stream::resume(std::chrono::microseconds fade_time) {
        if (m_stream) {
            m_stream->resume(fade_time);
        }
    }

    void music_stream::stop(std::chrono::microseconds fade_time) {
        if (m_stream) {
            m_stream->stop(fade_time);
        }
    }

    void music_stream::set_volume(float volume) {
        if (volume < 0.0f) volume = 0.0f;
        if (volume > 1.0f) volume = 1.0f;
        m_caller_volume = volume;
        if (m_stream) {
            m_stream->set_volume(m_caller_volume * music_group_volume());
        }
    }

    bool music_stream::is_playing() const {
        return m_stream ? m_stream->is_playing() : false;
    }

    bool music_stream::is_paused() const {
        return m_stream ? m_stream->is_paused() : false;
    }

    void music_stream::rewind() {
        if (m_stream) {
            m_stream->rewind();
        }
    }
}
