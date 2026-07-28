//
// Internal: marshal musac's audio-thread finish/loop callbacks to the main thread.
//
// musac invokes finish/loop callbacks on the audio thread under its callback mutex, with
// hard real-time rules (no allocation, no locks, be quick). A neutrino consumer callback
// wants to run on the main thread where it can touch game state, so the audio-thread hook
// only bumps a lock-free counter here; sound_system drains the counters during the app's
// per-frame update and invokes the consumer's std::function on the main thread.
//
// The relay is owned by shared_ptr and captured into the musac callback by shared_ptr, so a
// stream (and its owning wrapper) destroyed mid-flight can't leave the audio thread writing
// through a dangling pointer: the in-flight callback keeps the relay alive.
//

#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include <musac/stream.hh>

namespace neutrino::audio_detail {

    struct callback_relay {
        std::atomic<unsigned> finished{0};
        std::atomic<unsigned> looped{0};
    };

    /// Wire @p s so that finishing bumps @p relay->finished (audio-thread safe: one atomic add).
    inline void arm_finish(musac::audio_stream& s, std::shared_ptr<callback_relay> relay) {
        s.set_finish_callback([relay = std::move(relay)](musac::audio_stream&) {
            relay->finished.fetch_add(1, std::memory_order_relaxed);
        });
    }

    /// Wire @p s so that looping bumps @p relay->looped (audio-thread safe: one atomic add).
    inline void arm_loop(musac::audio_stream& s, std::shared_ptr<callback_relay> relay) {
        s.set_loop_callback([relay = std::move(relay)](musac::audio_stream&) {
            relay->looped.fetch_add(1, std::memory_order_relaxed);
        });
    }

    /// Drain @p relay->finished on the main thread, invoking @p cb once per pending finish.
    inline void drain_finished(callback_relay& relay, const std::function<void()>& cb) {
        const unsigned n = relay.finished.exchange(0, std::memory_order_relaxed);
        if (cb) {
            for (unsigned i = 0; i < n; ++i) {
                cb();
            }
        }
    }

    /// Drain @p relay->looped on the main thread, invoking @p cb once per pending loop.
    inline void drain_looped(callback_relay& relay, const std::function<void()>& cb) {
        const unsigned n = relay.looped.exchange(0, std::memory_order_relaxed);
        if (cb) {
            for (unsigned i = 0; i < n; ++i) {
                cb();
            }
        }
    }

} // namespace neutrino::audio_detail
