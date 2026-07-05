//
// Created by igor on 02/07/2026.
//

#include <neutrino/audio/pc_speaker.hh>
#include "sound_system.hh"
#include "services/service_locator.hh"
#include <musac/pc_speaker_stream.hh>

namespace neutrino {
    pc_speaker::pc_speaker()
        : m_stream(nullptr) {
    }

    pc_speaker::pc_speaker(std::unique_ptr<musac::pc_speaker_stream> stream)
        : m_stream(std::move(stream)) {
        if (m_stream) {
            m_stream->open();
        }
    }

    pc_speaker::~pc_speaker() {
        m_stream.reset();
    }

    pc_speaker::pc_speaker(pc_speaker&& other) noexcept = default;
    pc_speaker& pc_speaker::operator=(pc_speaker&& other) noexcept = default;

    void pc_speaker::play() {
        if (m_stream) {
            // Apply the sfx group volume for the PC speaker stream
            auto* ss = service_locator::instance().get_sound_system();
            m_stream->set_volume(ss ? ss->sfx_volume() : 1.0f);
            m_stream->play();
        }
    }

    void pc_speaker::pause() {
        if (m_stream) {
            m_stream->pause();
        }
    }

    void pc_speaker::stop() {
        if (m_stream) {
            m_stream->stop();
        }
    }

    void pc_speaker::sound(float frequency_hz, std::chrono::milliseconds duration) {
        if (m_stream) {
            m_stream->sound(frequency_hz, duration);
        }
    }

    void pc_speaker::beep(float frequency_hz) {
        if (m_stream) {
            m_stream->beep(frequency_hz);
        }
    }

    void pc_speaker::silence(std::chrono::milliseconds duration) {
        if (m_stream) {
            m_stream->silence(duration);
        }
    }

    void pc_speaker::clear_queue() {
        if (m_stream) {
            m_stream->clear_queue();
        }
    }

    bool pc_speaker::play_mml(const std::string& mml, bool strict) {
        if (m_stream) {
            return m_stream->play_mml(mml, strict);
        }
        return false;
    }

    bool pc_speaker::is_queue_empty() const {
        return m_stream ? m_stream->is_queue_empty() : true;
    }

    size_t pc_speaker::queue_size() const {
        return m_stream ? m_stream->queue_size() : 0;
    }
}
