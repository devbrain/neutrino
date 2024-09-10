#ifndef NEUTRINO_IMGUI_IMGUI_H
#define NEUTRINO_IMGUI_IMGUI_H

#include <neutrino/neutrino_export.hh>
#if !defined(IMGUI_API)
#define IMGUI_API NEUTRINO_EXPORT
#endif

#include <sdlpp/video/color.hh>
#include <sdlpp/video/geometry.hh>

#define IM_VEC4_CLASS_EXTRA                         \
ImVec4(const neutrino::sdl::color& c)               \
    :x(static_cast<float>(c.r)/255.0f),             \
     y(static_cast<float>(c.g)/255.0f),             \
     z(static_cast<float>(c.b)/255.0f),             \
     w(static_cast<float>(c.a)/255.0f) {}

#define IM_VEC2_CLASS_EXTRA                         \
    ImVec2(const neutrino::sdl::point& p)           \
        : x(p.x), y(p.y) {}                         \
    ImVec2(const neutrino::sdl::point2f& p)         \
        : x(p.x), y(p.y) {}

#include <neutrino/imgui/imgui_real.h>

#endif