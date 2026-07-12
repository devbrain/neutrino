//
// Created by igor on 12/07/2026.
//

#pragma once

//
// Internal RAII lock for reading a surface's pixels. Locks only when SDL_MUSTLOCK
// requires it; ready() is false only when a required lock failed (or the surface is
// null). Locks SDL directly rather than via sdlpp's surface::lock_guard, which misreads
// SDL3's bool return from SDL_LockSurface (see the risks note in the design doc).
//

#include <sdlpp/video/surface.hh>

namespace neutrino::details {
    class surface_pixel_lock {
        public:
            explicit surface_pixel_lock(const sdlpp::surface& s)
                : m_surface(s.get()) {
                if (!m_surface) {
                    m_ready = false;
                    return;
                }
                if (SDL_MUSTLOCK(m_surface)) {
                    m_locked = SDL_LockSurface(m_surface);
                    m_ready = m_locked;
                }
            }

            ~surface_pixel_lock() {
                if (m_locked) {
                    SDL_UnlockSurface(m_surface);
                }
            }

            surface_pixel_lock(const surface_pixel_lock&)            = delete;
            surface_pixel_lock& operator=(const surface_pixel_lock&) = delete;

            [[nodiscard]] bool ready() const noexcept { return m_ready; }

        private:
            SDL_Surface* m_surface{nullptr};
            bool         m_locked{false};
            bool         m_ready{true};
    };
}
