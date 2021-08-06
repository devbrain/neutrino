//
// Created by igor on 28/07/2021.
//

#ifndef NEUTRINO_ANIMATION_HH
#define NEUTRINO_ANIMATION_HH

#include "reader.hh"
#include <vector>
#include <chrono>

namespace neutrino::tiled::tmx
{
    class frame{
    public:
        frame (int tileid, std::chrono::milliseconds duration)
        : m_tileid(tileid), m_duration(duration) {}

        frame(frame&&) = default;

        [[nodiscard]] int id () const noexcept {
            return m_tileid;
        }

        [[nodiscard]] std::chrono::milliseconds duration () const noexcept {
            return m_duration;
        }
    private:
        int m_tileid;
        std::chrono::milliseconds m_duration;
    };

    class animation
    {
    public:
        static animation parse(const reader& elt);

        void add(frame f) {
            m_frames.emplace_back(std::move(f));
        }

        const std::vector<frame>& frames() const noexcept {
            return m_frames;
        }
    private:
        std::vector<frame> m_frames;
    };
}

#endif //NEUTRINO_ANIMATION_HH
