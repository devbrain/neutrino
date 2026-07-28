//
// Created by igor on 02/07/2026.
//

#include <neutrino/audio/sound_effect.hh>
#include "sound_system.hh"
#include "callback_relay.hh"
#include "sdl_io_stream.hh"
#include "memory_io.hh"
#include "services/service_access.hh"

#include <musac/audio_device.hh>
#include <musac/audio_source.hh>
#include <musac/stream.hh>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/decoders_registry.hh>
#include <algorithm>
#include <cstring>
#include <optional>
#include <vector>

namespace {
    // A musac decoder over a pre-decoded, device-rate, interleaved-float buffer: it hands
    // the samples straight back, so an audio_source built on it needs no decoding and (since
    // its rate equals the device rate) no resampler. The buffer is shared read-only, so each
    // concurrent channel gets its own decoder -- its own playback cursor -- over one copy.
    // This is neutrino's own codec; the musac library is untouched.
    class pcm_decoder final : public musac::decoder {
        public:
            pcm_decoder(std::shared_ptr<const std::vector<float>> pcm,
                        musac::sample_rate_t rate, musac::channels_t channels)
                : m_pcm(std::move(pcm)), m_rate(rate), m_channels(channels) {}

            [[nodiscard]] const char* get_name() const override { return "neutrino cached PCM"; }
            void open(musac::io_stream*) override { m_pos = 0; set_is_open(true); }
            [[nodiscard]] musac::channels_t get_channels() const override { return m_channels; }
            [[nodiscard]] musac::sample_rate_t get_rate() const override { return m_rate; }
            bool rewind() override { m_pos = 0; return true; }

            [[nodiscard]] std::chrono::microseconds duration() const override {
                const std::size_t frames = m_channels ? m_pcm->size() / m_channels : 0;
                return std::chrono::microseconds(
                    m_rate ? static_cast<std::int64_t>(frames) * 1'000'000 / m_rate : 0);
            }
            bool seek_to_time(std::chrono::microseconds t) override {
                const std::int64_t frame = m_rate ? t.count() * static_cast<std::int64_t>(m_rate) / 1'000'000 : 0;
                const std::size_t s = static_cast<std::size_t>(std::max<std::int64_t>(0, frame)) * m_channels;
                m_pos = std::min(s, m_pcm->size());
                return true;
            }

        protected:
            std::size_t do_decode(float* buf, std::size_t len, bool& call_again) override {
                const std::size_t remaining = m_pcm->size() - m_pos;
                const std::size_t todo = std::min(len, remaining);
                std::memcpy(buf, m_pcm->data() + m_pos, todo * sizeof(float));
                m_pos += todo;
                call_again = m_pos < m_pcm->size();
                return todo;
            }

        private:
            std::shared_ptr<const std::vector<float>> m_pcm; // interleaved, device rate/channels
            musac::sample_rate_t m_rate;
            musac::channels_t m_channels;
            std::size_t m_pos = 0; // interleaved-sample cursor
    };
}

namespace neutrino {

    namespace {
        struct captured_pcm {
            std::shared_ptr<const std::vector<float>> pcm;
            unsigned rate;
            unsigned channels;
        };

        // Decode @p data and resample it to the output device rate ONCE, returning the
        // interleaved device-rate float PCM. Runs the effect through musac's normal decode +
        // auto-resample pipeline a single time and captures its output; playback then needs no
        // per-channel decode or resampler. Returns nullopt if audio is inactive or the sample
        // can't be pre-rendered (no decoder, empty, or longer than a short effect) -- the
        // caller then falls back to per-play streaming. Music is never routed here (load_music).
        std::optional<captured_pcm> capture_device_pcm(sound_system& ss,
                                                       const std::shared_ptr<const std::vector<uint8_t>>& data) {
            if (!ss.active() || !ss.device()) {
                return std::nullopt;
            }
            const musac::sample_rate_t rate = ss.device()->get_freq();
            const musac::channels_t channels = ss.device()->get_channels();
            if (rate == 0 || channels == 0) {
                return std::nullopt;
            }
            auto io = audio_detail::io_from_buffer(data);
            if (!io) {
                return std::nullopt;
            }
            try {
                musac::audio_source src(std::move(io), ss.registry().get()); // real decoder + auto-resampler
                constexpr std::size_t frame_size = 4096;
                src.open(rate, channels, frame_size);

                auto pcm = std::make_shared<std::vector<float>>();
                std::vector<float> chunk(frame_size * channels);
                const std::size_t cap = static_cast<std::size_t>(rate) * channels * 30; // 30s guard
                for (;;) {
                    std::size_t pos = 0;
                    src.read_samples(chunk.data(), pos, chunk.size(), channels);
                    if (pos == 0) {
                        break; // drained
                    }
                    pcm->insert(pcm->end(), chunk.begin(), chunk.begin() + static_cast<std::ptrdiff_t>(pos));
                    if (pcm->size() >= cap) {
                        // Too long to be a short effect: discard the partial capture and fall
                        // back to per-play streaming rather than caching a truncated effect.
                        return std::nullopt;
                    }
                }
                if (pcm->empty()) {
                    return std::nullopt;
                }
                return captured_pcm{std::const_pointer_cast<const std::vector<float>>(pcm),
                                    static_cast<unsigned>(rate), static_cast<unsigned>(channels)};
            } catch (const std::exception&) {
                return std::nullopt; // e.g. no decoder for the format -> stream on demand instead
            }
        }
    } // namespace

    static void register_self(sound_effect* effect) {
        if (auto* ss = maybe_sound_system()) {
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

    sound_effect::sound_effect(std::shared_ptr<const std::vector<float>> pcm, unsigned rate, unsigned channels)
        : m_pcm(std::move(pcm)), m_pcm_rate(rate), m_pcm_channels(channels) {
        register_self(this);
    }

    sound_effect::~sound_effect() {
        if (auto* ss = maybe_sound_system()) {
            ss->unregister_effect(this);
        }
        m_channels.clear();
    }

    sound_effect::sound_effect(sound_effect&& other) noexcept
        : m_path(std::move(other.m_path)),
          m_data(std::move(other.m_data)),
          m_pcm(std::move(other.m_pcm)),
          m_pcm_rate(other.m_pcm_rate),
          m_pcm_channels(other.m_pcm_channels),
          m_pcm_tried(other.m_pcm_tried),
          m_source(std::move(other.m_source)),
          m_channels(std::move(other.m_channels)),
          m_relay(std::move(other.m_relay)),
          m_on_finished(std::move(other.m_on_finished)) {
        // other stays registered until its destructor runs; its channel
        // list is empty now, so that registration is harmless.
        register_self(this);
    }

    sound_effect& sound_effect::operator=(sound_effect&& other) noexcept {
        // Effects register *unconditionally* in every constructor and by stable address,
        // which move-assignment does not change -- so the target stays registered and only
        // the data members move (self-assign guarded). This is behaviourally the same as a
        // defaulted move-assign, spelled out to match the hand-written move constructor and
        // to make the "no (un)register needed" reasoning explicit (cf. music_stream, whose
        // registration is conditional and does need the discipline).
        if (this != &other) {
            m_path         = std::move(other.m_path);
            m_data         = std::move(other.m_data);
            m_pcm          = std::move(other.m_pcm);
            m_pcm_rate     = other.m_pcm_rate;
            m_pcm_channels = other.m_pcm_channels;
            m_pcm_tried    = other.m_pcm_tried;
            m_source       = std::move(other.m_source);
            m_channels     = std::move(other.m_channels);
            m_relay        = std::move(other.m_relay);
            m_on_finished  = std::move(other.m_on_finished);
        }
        return *this;
    }

    void sound_effect::apply_group_volume(float group) {
        for (auto& c : m_channels) {
            if (c.stream) {
                c.stream->set_volume(c.caller_volume * group);
            }
        }
    }

    void sound_effect::play(float volume, std::chrono::microseconds fade_time) {
        auto* ss = maybe_sound_system();
        if (!ss || !ss->active()) {
            return;
        }
        // The effect may predate the application (registration is idempotent).
        ss->register_effect(this);
        const float group = ss->sfx_volume();

        // Lazy resample-and-cache: the first time a data-backed effect plays, decode +
        // resample it to device-rate PCM once, so this and every later play (and overlapping
        // channels) need no resampler. Attempted at most once -- if it can't be pre-rendered,
        // m_data is kept and we fall through to the per-play streaming branch below.
        if (!m_pcm && m_data && !m_pcm_tried) {
            m_pcm_tried = true;
            if (auto cap = capture_device_pcm(*ss, m_data)) {
                m_pcm          = cap->pcm;
                m_pcm_rate     = cap->rate;
                m_pcm_channels = cap->channels;
            }
        }

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
        } else if (m_pcm) {
            // Pre-decoded device-rate PCM: play it straight through neutrino's own PCM
            // decoder -- no per-channel decode, and no resampler (rate == device rate). The
            // decoder ignores the io_stream, but the source ctor still wants one.
            static const auto s_dummy_io = std::make_shared<const std::vector<uint8_t>>(1, uint8_t{0});
            auto decoder = std::make_unique<pcm_decoder>(
                m_pcm, static_cast<musac::sample_rate_t>(m_pcm_rate),
                static_cast<musac::channels_t>(m_pcm_channels));
            musac::audio_source source(std::move(decoder), audio_detail::io_from_buffer(s_dummy_io));
            auto stream = ss->device()->create_stream(std::move(source));
            stream.open();
            arm_channel(stream);
            stream.set_volume(volume * group);
            stream.play(1, fade_time);
            m_channels.push_back({std::make_unique<musac::audio_stream>(std::move(stream)), volume});
        } else {
            // Create a new channel: each concurrent channel decodes its own
            // view of the source (a fresh file handle or memory-buffer view).
            auto io = !m_path.empty() ? audio_detail::io_from_file(m_path)
                                      : audio_detail::io_from_buffer(m_data);
            if (io) {
                musac::audio_source source(std::move(io), ss->registry().get());
                auto stream = ss->device()->create_stream(std::move(source));
                stream.open();
                arm_channel(stream);
                stream.set_volume(volume * group);
                stream.play(1, fade_time);

                m_channels.push_back({std::make_unique<musac::audio_stream>(std::move(stream)), volume});
            } else if (m_source) {
                // For custom sources, we can only play a single stream since audio_source is move-only.
                // We create it on the first play call.
                if (m_channels.empty()) {
                    auto stream = ss->device()->create_stream(std::move(*m_source));
                    stream.open();
                    arm_channel(stream);
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

    void sound_effect::on_finished(std::function<void()> cb) {
        m_on_finished = std::move(cb);
    }

    void sound_effect::arm_channel(musac::audio_stream& stream) {
        if (!m_relay) {
            m_relay = std::make_shared<audio_detail::callback_relay>();
        }
        audio_detail::arm_finish(stream, m_relay);
    }

    void sound_effect::dispatch_pending() {
        if (!m_relay) {
            return;
        }
        audio_detail::drain_finished(*m_relay, m_on_finished);
    }
}
