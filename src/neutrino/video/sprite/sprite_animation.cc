//
// Created by igor on 05/07/2026.
//

#include <neutrino/video/sprite/sprite_animation.hh>

#include <algorithm>
#include <cmath>
#include <utility>

#include <failsafe/enforce.hh>

#include "services/service_access.hh"
#include "video/sprite/sprites_manager.hh"

namespace neutrino {
    namespace {
        [[nodiscard]] sprite_animation_duration clamp_or_wrap(
            sprite_animation_duration elapsed,
            sprite_animation_duration total,
            bool loop) noexcept {
            if (elapsed <= sprite_animation_duration::zero()) {
                return sprite_animation_duration::zero();
            }

            if (!loop) {
                return elapsed;
            }

            const auto total_ms = total.count();
            if (total_ms <= 0.0f) {
                return sprite_animation_duration::zero();
            }

            return sprite_animation_duration{std::fmod(elapsed.count(), total_ms)};
        }
    }

    sprite_animation::sprite_animation(bool loop)
        : m_loop(loop) {
    }

    sprite_animation::sprite_animation(std::vector <sprite_animation_frame> frames, bool loop)
        : m_frames(std::move(frames)),
          m_loop(loop) {
        for (const auto& frame : m_frames) {
            validate_frame(frame);
        }
        rebuild_frame_index();
    }

    bool sprite_animation::loop() const noexcept {
        return m_loop;
    }

    void sprite_animation::set_loop(bool loop) noexcept {
        m_loop = loop;
    }

    std::size_t sprite_animation::frame_count() const noexcept {
        return m_frames.size();
    }

    bool sprite_animation::empty() const noexcept {
        return m_frames.empty();
    }

    sprite_animation_duration sprite_animation::total_duration() const noexcept {
        return m_total_duration;
    }

    const sprite_animation_frame& sprite_animation::frame(std::size_t index) const {
        ENFORCE(index < m_frames.size());
        return m_frames[index];
    }

    void sprite_animation::add_frame(sprite_animation_frame frame) {
        validate_frame(frame);
        m_total_duration += frame.duration;
        m_frames.push_back(std::move(frame));
        m_end_times.push_back(m_total_duration);
    }

    const sprite_animation_frame& sprite_animation::frame_at(sprite_animation_duration elapsed) const {
        ENFORCE(!m_frames.empty());

        const auto normalized = clamp_or_wrap(elapsed, m_total_duration, m_loop);
        const auto it = std::upper_bound(m_end_times.begin(), m_end_times.end(), normalized);
        const auto index = std::min(
            static_cast <std::size_t>(std::distance(m_end_times.begin(), it)),
            m_frames.size() - 1);
        return m_frames[index];
    }

    sprite_appearance sprite_animation::appearance_at(sprite_animation_duration elapsed) const {
        return frame_at(elapsed).appearance;
    }

    void sprite_animation::validate_frame(const sprite_animation_frame& frame) {
        ENFORCE(frame.duration > sprite_animation_duration::zero());
    }

    void sprite_animation::rebuild_frame_index() {
        m_total_duration = sprite_animation_duration::zero();
        m_end_times.clear();
        m_end_times.reserve(m_frames.size());
        for (const auto& frame : m_frames) {
            m_total_duration += frame.duration;
            m_end_times.push_back(m_total_duration);
        }
    }

    sprite_animation_id register_sprite_animation(sprite_animation animation) {
        return require_sprites_manager().create(std::move(animation));
    }

    void unregister_sprite_animation(sprite_animation_id animation) {
        if (!animation.valid()) {
            return;
        }
        // Services already torn down: the resource is gone, nothing to do.
        if (auto* manager = maybe_sprites_manager()) {
            manager->erase(animation);
        }
    }
}
