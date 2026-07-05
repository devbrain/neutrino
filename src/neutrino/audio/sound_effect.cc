//
// Created by igor on 02/07/2026.
//

#include <neutrino/audio/sound_effect.hh>
#include "sound_system.hh"
#include "sdl_io_stream.hh"
#include "memory_io.hh"
#include "services/service_locator.hh"

#include <musac/audio_device.hh>
#include <musac/audio_source.hh>
#include <musac/stream.hh>
#include <musac/sdk/decoders_registry.hh>
#include <algorithm>

namespace neutrino {
    static sound_system* system_ptr() {
        return service_locator::instance().get_sound_system();
    }

    static void register_self(sound_effect* effect) {
        if (auto* ss = system_ptr()) {
            ss->register_effect(effect);
        }
    }

    sound_effect::sound_effect()
        : m_source(nullptr) {
        register_self(this);
    }

    sound_effect::sound_effect(std::string path)
        : m_path(std::move(path)), m_source(nullptr) {
        register_self(this);
    }

    sound_effect::sound_effect(std::shared_ptr<musac::audio_source> source)
        : m_source(std::move(source)) {
        register_self(this);
    }

    sound_effect::sound_effect(std::shared_ptr<const std::vector<uint8_t>> data)
        : m_data(std::move(data)) {
        register_self(this);
    }

    sound_effect::~sound_effect() {
        if (auto* ss = system_ptr()) {
            ss->unregister_effect(this);
        }
        m_channels.clear();
    }

    sound_effect::sound_effect(sound_effect&& other) noexcept
        : m_path(std::move(other.m_path)),
          m_data(std::move(other.m_data)),
          m_source(std::move(other.m_source)),
          m_channels(std::move(other.m_channels)) {
        // other stays registered until its destructor runs; its channel
        // list is empty now, so that registration is harmless.
        register_self(this);
    }

    sound_effect& sound_effect::operator=(sound_effect&& other) noexcept = default;

    void sound_effect::apply_group_volume(float group) {
        for (auto& c : m_channels) {
            if (c.stream) {
                c.stream->set_volume(c.caller_volume * group);
            }
        }
    }

    void sound_effect::play(float volume, std::chrono::microseconds fade_time) {
        auto* ss = system_ptr();
        if (!ss || !ss->active()) {
            return;
        }
        // The effect may predate the application (registration is idempotent).
        ss->register_effect(this);
        const float group = ss->sfx_volume();

        // Clean up finished streams if there are any that were destroyed/invalidated
        m_channels.erase(
            std::remove_if(m_channels.begin(), m_channels.end(), [](const auto& c) {
                return c.stream == nullptr;
            }),
            m_channels.end()
        );

        // Find an idle channel
        channel* idle_channel = nullptr;
        for (auto& c : m_channels) {
            if (c.stream && !c.stream->is_playing() && !c.stream->is_paused()) {
                idle_channel = &c;
                break;
            }
        }

        if (idle_channel) {
            idle_channel->caller_volume = volume;
            idle_channel->stream->rewind();
            idle_channel->stream->set_volume(volume * group);
            idle_channel->stream->play(1, fade_time);
        } else {
            // Create a new channel: each concurrent channel decodes its own
            // view of the source (a fresh file handle or memory-buffer view).
            auto io = !m_path.empty() ? audio_detail::io_from_file(m_path)
                                      : audio_detail::io_from_buffer(m_data);
            if (io) {
                musac::audio_source source(std::move(io), ss->registry().get());
                auto stream = ss->device()->create_stream(std::move(source));
                stream.open();
                stream.set_volume(volume * group);
                stream.play(1, fade_time);

                m_channels.push_back({std::make_unique<musac::audio_stream>(std::move(stream)), volume});
            } else if (m_source) {
                // For custom sources, we can only play a single stream since audio_source is move-only.
                // We create it on the first play call.
                if (m_channels.empty()) {
                    auto stream = ss->device()->create_stream(std::move(*m_source));
                    stream.open();
                    stream.set_volume(volume * group);
                    stream.play(1, fade_time);

                    m_channels.push_back({std::make_unique<musac::audio_stream>(std::move(stream)), volume});
                } else {
                    auto& c = m_channels[0];
                    if (c.stream) {
                        if (!c.stream->is_playing()) {
                            c.stream->rewind();
                        }
                        c.caller_volume = volume;
                        c.stream->set_volume(volume * group);
                        c.stream->play(1, fade_time);
                    }
                }
            }
        }
    }

    void sound_effect::stop(std::chrono::microseconds fade_time) {
        for (auto& c : m_channels) {
            if (c.stream) {
                c.stream->stop(fade_time);
            }
        }
    }

    bool sound_effect::is_playing() const {
        for (const auto& c : m_channels) {
            if (c.stream && c.stream->is_playing()) {
                return true;
            }
        }
        return false;
    }
}
