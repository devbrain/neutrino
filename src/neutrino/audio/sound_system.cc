//
// Created by igor on 02/07/2026.
//

#include <algorithm>
#include <istream>

#include <neutrino/audio/audio.hh>
#include "sound_system.hh"
#include "sdl_io_stream.hh"
#include "memory_io.hh"
#include "services/service_locator.hh"

#include <musac_backends/sdl3/sdl3_backend.hh>
#include <musac/audio_device.hh>
#include <musac/audio_source.hh>
#include <musac/audio_system.hh>
#include <musac/stream.hh>
#include <musac/pc_speaker_stream.hh>
#include <musac/codecs/register_codecs.hh>
#include <musac/sdk/decoders_registry.hh>
#include <failsafe/enforce.hh>
#include <failsafe/logger.hh>

namespace neutrino {
    static float clamp_volume(float volume) {
        if (volume < 0.0f) return 0.0f;
        if (volume > 1.0f) return 1.0f;
        return volume;
    }

    sound_system::sound_system() {
        try {
            m_backend = musac::create_sdl3_backend();
            m_codecs = musac::create_registry_with_all_codecs();
            if (!musac::audio_system::init(m_backend, m_codecs)) {
                LOG_ERROR("Failed to initialize audio system, audio is disabled");
                return;
            }
            m_system_up = true;
            auto device = musac::audio_device::open_default_device(m_backend);
            m_device = std::make_unique <musac::audio_device>(std::move(device));
            m_device->set_gain(m_master_volume);
        } catch (const std::exception& ex) {
            LOG_ERROR("No audio device available, audio is disabled:", ex.what());
            m_device.reset();
        }
    }

    sound_system::~sound_system() {
        m_retiring_music.reset();
        m_music_slot.reset();
        m_device.reset();
        if (m_system_up) {
            musac::audio_system::done();
        }
    }

    bool sound_system::active() const {
        return m_device != nullptr;
    }

    musac::audio_device* sound_system::device() const {
        return m_device.get();
    }

    std::shared_ptr <musac::decoders_registry> sound_system::registry() const {
        return m_codecs;
    }

    void sound_system::set_master_volume(float volume) {
        m_master_volume = clamp_volume(volume);
        if (m_device) {
            m_device->set_gain(m_master_volume);
        }
    }

    float sound_system::master_volume() const {
        return m_master_volume;
    }

    void sound_system::set_sfx_volume(float volume) {
        m_sfx_volume = clamp_volume(volume);
        for (auto* effect : m_effects) {
            effect->apply_group_volume(m_sfx_volume);
        }
    }

    float sound_system::sfx_volume() const {
        return m_sfx_volume;
    }

    void sound_system::set_music_volume(float volume) {
        m_music_volume = clamp_volume(volume);
        if (m_music_slot) {
            m_music_slot->set_volume(m_music_volume);
        }
        for (auto* music : m_musics) {
            music->apply_group_volume(m_music_volume);
        }
    }

    float sound_system::music_volume() const {
        return m_music_volume;
    }

    void sound_system::play_music(const std::string& path, bool loop, std::chrono::microseconds fade_time) {
        if (!active()) return;

        auto io = audio_detail::io_from_file(path);
        ENFORCE(io != nullptr)("Failed to open audio file: " + path);
        play_music(std::move(io), loop, fade_time);
    }

    void sound_system::play_music(std::unique_ptr <musac::io_stream> io, bool loop, std::chrono::microseconds fade_time) {
        if (!active() || !io) return;

        musac::audio_source source(std::move(io), m_codecs.get());
        auto stream = m_device->create_stream(std::move(source));
        auto next = std::make_unique <musac::audio_stream>(std::move(stream));
        next->open();

        if (m_music_slot) {
            m_music_slot->stop(fade_time);
            // Keep the old track alive so its fade-out is audible; it is
            // released on the next replacement.
            m_retiring_music = std::move(m_music_slot);
        }
        m_music_slot = std::move(next);
        m_music_slot->set_volume(m_music_volume);
        m_music_slot->play(loop ? 0 : 1, fade_time);
    }

    void sound_system::stop_music(std::chrono::microseconds fade_time) {
        if (m_music_slot) {
            m_music_slot->stop(fade_time);
            m_retiring_music = std::move(m_music_slot);
        }
    }

    void sound_system::pause_music(std::chrono::microseconds fade_time) {
        if (m_music_slot) {
            m_music_slot->pause(fade_time);
        }
    }

    void sound_system::resume_music(std::chrono::microseconds fade_time) {
        if (m_music_slot) {
            m_music_slot->resume(fade_time);
        }
    }

    bool sound_system::music_playing() const {
        return m_music_slot && m_music_slot->is_playing();
    }

    void sound_system::register_effect(sound_effect* effect) {
        if (std::find(m_effects.begin(), m_effects.end(), effect) == m_effects.end()) {
            m_effects.push_back(effect);
        }
    }

    void sound_system::unregister_effect(sound_effect* effect) {
        m_effects.erase(std::remove(m_effects.begin(), m_effects.end(), effect), m_effects.end());
    }

    void sound_system::register_music(music_stream* music) {
        if (std::find(m_musics.begin(), m_musics.end(), music) == m_musics.end()) {
            m_musics.push_back(music);
        }
    }

    void sound_system::unregister_music(music_stream* music) {
        m_musics.erase(std::remove(m_musics.begin(), m_musics.end(), music), m_musics.end());
    }

    // ------------------------------------------------------------------
    // Public free functions
    // ------------------------------------------------------------------
    static sound_system* system_ptr() {
        return service_locator::instance().get_sound_system();
    }

    bool audio_active() {
        auto* ss = system_ptr();
        return ss && ss->active();
    }

    void set_master_volume(float volume) {
        if (auto* ss = system_ptr()) ss->set_master_volume(volume);
    }

    float get_master_volume() {
        auto* ss = system_ptr();
        return ss ? ss->master_volume() : 1.0f;
    }

    void set_sfx_volume(float volume) {
        if (auto* ss = system_ptr()) ss->set_sfx_volume(volume);
    }

    float get_sfx_volume() {
        auto* ss = system_ptr();
        return ss ? ss->sfx_volume() : 1.0f;
    }

    void set_music_volume(float volume) {
        if (auto* ss = system_ptr()) ss->set_music_volume(volume);
    }

    float get_music_volume() {
        auto* ss = system_ptr();
        return ss ? ss->music_volume() : 1.0f;
    }

    sound_effect load_sfx(const std::string& path) {
        // Validate the path eagerly so a typo surfaces at load time, not at
        // the first play() deep inside the game.
        auto io = audio_detail::io_from_file(path);
        ENFORCE(io != nullptr)("Failed to open audio file: " + path);
        return sound_effect(path);
    }

    music_stream load_music(const std::string& path) {
        auto io = audio_detail::io_from_file(path);
        ENFORCE(io != nullptr)("Failed to open audio file: " + path);

        auto* ss = system_ptr();
        if (!ss || !ss->active()) {
            return music_stream{};
        }
        musac::audio_source source(std::move(io), ss->registry().get());
        auto stream = ss->device()->create_stream(std::move(source));
        return music_stream(std::make_unique <musac::audio_stream>(std::move(stream)));
    }

    sound_effect load_sfx(std::istream& is) {
        auto data = audio_detail::read_all(is);
        ENFORCE(!data->empty())("Failed to read audio data from stream");
        return sound_effect(std::move(data));
    }

    music_stream load_music(std::istream& is) {
        auto data = audio_detail::read_all(is);
        ENFORCE(!data->empty())("Failed to read audio data from stream");

        auto* ss = system_ptr();
        if (!ss || !ss->active()) {
            return music_stream{};
        }
        musac::audio_source source(audio_detail::io_from_buffer(std::move(data)), ss->registry().get());
        auto stream = ss->device()->create_stream(std::move(source));
        return music_stream(std::make_unique <musac::audio_stream>(std::move(stream)));
    }

    pc_speaker create_pc_speaker() {
        auto* ss = system_ptr();
        if (!ss || !ss->active()) {
            return pc_speaker{};
        }
        return pc_speaker(std::unique_ptr <musac::pc_speaker_stream>(
            new musac::pc_speaker_stream(ss->device()->create_pc_speaker_stream())
        ));
    }

    void play_music(const std::string& path, bool loop, std::chrono::microseconds fade_time) {
        if (auto* ss = system_ptr()) ss->play_music(path, loop, fade_time);
    }

    void play_music(std::istream& is, bool loop, std::chrono::microseconds fade_time) {
        auto data = audio_detail::read_all(is);
        ENFORCE(!data->empty())("Failed to read audio data from stream");
        if (auto* ss = system_ptr()) {
            ss->play_music(audio_detail::io_from_buffer(std::move(data)), loop, fade_time);
        }
    }

    void stop_music(std::chrono::microseconds fade_time) {
        if (auto* ss = system_ptr()) ss->stop_music(fade_time);
    }

    void pause_music(std::chrono::microseconds fade_time) {
        if (auto* ss = system_ptr()) ss->pause_music(fade_time);
    }

    void resume_music(std::chrono::microseconds fade_time) {
        if (auto* ss = system_ptr()) ss->resume_music(fade_time);
    }

    bool is_music_playing() {
        auto* ss = system_ptr();
        return ss && ss->music_playing();
    }

    void register_decoder(
        const std::function <bool(musac::io_stream*)>& accept_func,
        const std::function <std::unique_ptr <musac::decoder>()>& factory_func,
        int priority
    ) {
        auto* ss = system_ptr();
        ENFORCE(ss != nullptr)("Cannot register decoder: application is not running");
        auto registry = ss->registry();
        ENFORCE(registry != nullptr)("Decoders registry is null");
        registry->register_decoder(accept_func, factory_func, priority);
    }
}
