//
// Created by igor on 03/06/2020.
//

#ifndef NEUTRINO_SDL_GEOMETRY_HH
#define NEUTRINO_SDL_GEOMETRY_HH

#include <neutrino/sdl/sdl2.hh>
#include <utility>
#include <vector>
#include <utility>
#include <array>
#include <cstdint>

namespace neutrino::sdl
{
    struct point : public SDL_Point
    {
        point() noexcept;
        point(int x_, int y_) noexcept;
        explicit point(const SDL_Point& p) noexcept;
        point& operator=(const SDL_Point& p) noexcept;
    };

    using dimensions = point;

    std::ostream& operator<<(std::ostream& os, const point& p);

    struct rect : public SDL_Rect
    {
        rect() noexcept;
        rect(int x_, int y_, int w_, int h_) noexcept;
        rect(const point& p, int w_, int h_) noexcept;
        explicit rect(const SDL_Rect& r) noexcept;
        rect& operator=(const SDL_Rect& r) noexcept;

        [[nodiscard]] bool inside(const point& p) const noexcept;
        [[nodiscard]] bool inside(const point* points, std::size_t size) const noexcept;

        template<typename PointsContainer>
        [[nodiscard]] bool inside(const PointsContainer& points_contanier) const noexcept;

        [[nodiscard]] rect enclose(const point* points, std::size_t size) const noexcept;

        template<typename PointsContainer>
        [[nodiscard]] rect enclose(const PointsContainer& points_contanier) const noexcept;

        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] bool equals(const rect& other) const noexcept;
        [[nodiscard]] bool intersects(const rect& other) const noexcept;
        [[nodiscard]] bool intersects(const point& a, const point& b) const noexcept;
        [[nodiscard]] rect intersection(const rect& other) const noexcept;
        [[nodiscard]] std::pair<point, point> intersection(const point& a, const point& b) const noexcept;
        [[nodiscard]] rect union_rect(const rect& other) const noexcept;
    };

    [[nodiscard]] inline bool inside(const rect& r, const point& p) noexcept
    {
        return SDL_TRUE == SDL_PointInRect(&p, &r);
    }

    [[nodiscard]] inline bool empty(const rect& r) noexcept
    {
        return SDL_TRUE == SDL_RectEmpty(&r);
    }

    [[nodiscard]] inline bool equals(const rect& a, const rect& b) noexcept
    {
        return SDL_TRUE == SDL_RectEquals(&a, &b);
    }

    [[nodiscard]] inline bool operator==(const rect& a, const rect& b) noexcept
    {
        return equals(a, b);
    }

    [[nodiscard]] inline bool intersects(const rect& a, const rect& b) noexcept
    {
        return SDL_HasIntersection(&a, &b);
    }

    [[nodiscard]] inline rect intersection(const rect& a, const rect& b) noexcept
    {
        rect result;
        SDL_IntersectRect(&a, &b, &result);
        return result;
    }
    // intersection between rect and line
    [[nodiscard]] inline std::pair<point, point> intersection(const rect& r, const point& a, const point& b) noexcept
    {
        std::pair<point, point> ret;
        int x1 = a.x;
        int y1 = a.y;
        int x2 = b.x;
        int y2 = b.y;

        if (SDL_TRUE == SDL_IntersectRectAndLine(&r, &x1, &y1, &x2, &y2))
        {
            ret.first = point(x1, y1);
            ret.second = point(x2, y2);
        }
        return ret;
    }
    // intersection between rect and line
    [[nodiscard]] inline bool intersects(const rect& r, const point& a, const point& b) noexcept
    {
        std::pair<point, point> ret;
        int x1 = a.x;
        int y1 = a.y;
        int x2 = b.x;
        int y2 = b.y;

        return (SDL_TRUE == SDL_IntersectRectAndLine(&r, &x1, &y1, &x2, &y2));
    }

    [[nodiscard]] inline rect union_rect(const rect& a, const rect& b) noexcept
    {
        rect result;
        SDL_UnionRect(&a, &b, &result);
        return result;
    }

    // ==================================================================
    template<typename T>
    struct points_container_traits;

    template<class Allocator>
    struct points_container_traits<std::vector<point, Allocator>>
    {
        using container = std::vector<point, Allocator>;
        static const point* data(const container& c)
        {
            return c.data();
        }
        static std::size_t size(const container& c)
        {
            return c.size();
        }
    };

    template<std::size_t N>
    struct points_container_traits<std::array<point, N>>
    {
        using container = std::array<point, N>;
        static const point* data(const container& c)
        {
            return c.data();
        }
        static std::size_t size(const container& c)
        {
            return c.size();
        }
    };

    [[nodiscard]] inline bool enclose(const point* points, std::size_t size, const rect& clip, rect& result) noexcept
    {
        return SDL_TRUE == SDL_EnclosePoints(points,
                                             static_cast<int>(size),
                                             &clip, &result);
    }

    template<typename PointsContainer>
    [[nodiscard]] inline bool enclose(const PointsContainer& points_contanier, const rect& clip, rect& result) noexcept
    {
        return enclose(points_container_traits<PointsContainer>::data(points_contanier),
                       static_cast<int>(points_container_traits<PointsContainer>::size(points_contanier)),
                       &clip, &result);
    }

    [[nodiscard]] inline bool inside(const rect& clip, const point* points, std::size_t size) noexcept
    {
        rect r;
        return enclose(points, size, clip, r);
    }

    template<typename PointsContainer>
    [[nodiscard]] inline bool inside(const rect& clip, const PointsContainer& points_contanier) noexcept
    {
        rect r;
        return enclose(points_contanier, clip, r);
    }
} // ns sdl
// ======================================================================
// Implementation
// ======================================================================
namespace neutrino::sdl
{
    inline point::point() noexcept
            : SDL_Point{0, 0}
    {
    }
    // -------------------------------------------------------------------
    inline point::point(int x_, int y_) noexcept
            : SDL_Point{x_, y_}
    {
    }
    // -------------------------------------------------------------------
    inline point::point(const SDL_Point& p) noexcept
            : SDL_Point{p.x, p.y}
    {
    }
    // -------------------------------------------------------------------
    inline point& point::operator=(const SDL_Point& p) noexcept
    {
        x = p.x;
        y = p.y;
        return *this;
    }
    // ===================================================================
    inline rect::rect() noexcept
            : SDL_Rect{0, 0, 0, 0}
    {
    }
    // ------------------------------------------------------------------
    inline rect::rect(int x_, int y_, int w_, int h_) noexcept
            : SDL_Rect{x_, y_, w_, h_}
    {
    }
    // ------------------------------------------------------------------
    inline rect::rect(const point& p, int w_, int h_) noexcept
            : SDL_Rect{p.x, p.y, w_, h_}
    {

    }
    // ------------------------------------------------------------------
    inline rect::rect(const SDL_Rect& r) noexcept
            : SDL_Rect{r.x, r.y, r.w, r.h}
    {
    }
    // ------------------------------------------------------------------
    inline rect& rect::operator=(const SDL_Rect& r) noexcept
    {
        x = r.x;
        y = r.y;
        w = r.w;
        h = r.h;
        return *this;
    }
    inline bool rect::inside(const point& p) const noexcept
    {
        return sdl::inside(*this, p);
    }
    // -------------------------------------------------------------------
    inline bool rect::empty() const noexcept
    {
        return sdl::empty(*this);
    }
    // -------------------------------------------------------------------
    inline bool rect::equals(const rect& other) const noexcept
    {
        return sdl::equals(*this, other);
    }
    // -------------------------------------------------------------------
    inline bool rect::intersects(const rect& other) const noexcept
    {
        return sdl::intersects(*this, other);
    }
    // -------------------------------------------------------------------
    inline bool rect::intersects(const point& a, const point& b) const noexcept
    {
        return sdl::intersects(*this, a, b);
    }
    // -------------------------------------------------------------------
    inline rect rect::intersection(const rect& other) const noexcept
    {
        return sdl::intersection(*this, other);
    }
    // -------------------------------------------------------------------
    inline std::pair<point, point> rect::intersection(const point& a, const point& b) const noexcept
    {
        return sdl::intersection(*this, a, b);
    }
    // -------------------------------------------------------------------
    inline rect rect::union_rect(const rect& other) const noexcept
    {
        return sdl::union_rect(*this, other);
    }
    // -------------------------------------------------------------------
    inline bool rect::inside(const point* points, std::size_t size) const noexcept
    {
        return sdl::inside(*this, points, size);
    }
    // -------------------------------------------------------------------
    template<typename PointsContainer>
    inline bool rect::inside(const PointsContainer& points_contanier) const noexcept
    {
        return sdl::inside(*this, points_contanier);
    }
    // -------------------------------------------------------------------
    inline rect rect::enclose(const point* points, std::size_t size) const noexcept
    {
        rect rc;
        (void) sdl::enclose(points, size, *this, rc);
        return rc;
    }
    // -------------------------------------------------------------------
    template<typename PointsContainer>
    inline rect rect::enclose(const PointsContainer& points_contanier) const noexcept
    {
        rect rc;
        sdl::enclose(points_contanier, *this, rc);
        return rc;
    }
} // ns sdl


#endif
