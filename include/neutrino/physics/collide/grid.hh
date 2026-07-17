//
// Created by igor on 24/06/2026.
//

/**
 * @file grid.hh
 * @brief A uniform 2D spatial grid (@ref neutrino::physics::grid) and its sparse cell storage --
 *        the static-tilemap broadphase behind @ref neutrino::physics::world.
 *
 * The grid divides an axis-aligned physical box into a @c W x @c H array of equal cells, each
 * holding at most one payload @c T (a tile). It maps world coordinates to cells, stores cells
 * sparsely (a dense payload vector indexed through a flat coord table, with a free list), and
 * answers the three broadphase traversals the world needs:
 *   - @ref neutrino::physics::grid::query -- the occupied cells overlapping an AABB region.
 *   - @ref neutrino::physics::grid::swept -- the cells a moving shape's bound sweeps over a frame.
 *   - @ref neutrino::physics::grid::raycast -- the occupied cells a segment crosses, near-to-far,
 *     via an Amanatides-Woo DDA (with supercover handling for exact gridline/corner crossings).
 *
 * Cells are addressed two ways. Queries see only geometry (the cell's world-space box); the grid's
 * OWNER (the world) gets a stable, positional **linear cell handle** (@ref
 * neutrino::physics::grid::to_cell / @ref neutrino::physics::grid::at) plus a per-cell **generation**
 * counter, so a handle to an overwritten or cleared cell reads as stale rather than aliasing a new
 * tile. @ref neutrino::physics::grid::compile_runs is a generic greedy rectangle-merge pass the
 * world uses to bake adjacent tiles into bigger colliders.
 *
 * @note The grid is a generic container over an arbitrary payload @c T; all tile/material/filter
 *       semantics live in the owner (see @ref world.hh). Construct via @ref
 *       neutrino::physics::grid::from_tile_size for a tilemap-exact cell size.
 */

#pragma once

#include <algorithm>
#include <utility>
#include <vector>
#include <cstdint>
#include <cmath>
#include <limits>
#include <type_traits>

#include <neutrino/physics/geometry/shapes.hh>
#include <neutrino/physics/geometry/clip.hh>
#include <failsafe/enforce.hh>

namespace neutrino::physics {
    namespace detail {
        /**
         * @brief An integer cell coordinate @c (x, y) with an INVALID sentinel.
         *
         * A default-constructed coord is INVALID (the "outside the grid" / null result); a coord
         * tests @c true via @c explicit @c operator bool only when both components are in range.
         */
        struct grid_coord {
            static constexpr auto INVALID = std::numeric_limits <uint32_t>::max(); ///< Out-of-grid / null sentinel.
            uint32_t x; ///< Column index (0-based), or @ref INVALID when out of grid.
            uint32_t y; ///< Row index (0-based), or @ref INVALID when out of grid.

            /// @brief Construct the INVALID (null) coord.
            grid_coord()
                : x(INVALID), y(INVALID) {
            }

            /// @brief Construct from explicit column @p x_ and row @p y_.
            grid_coord(uint32_t x_, uint32_t y_)
                : x(x_), y(y_) {
            }

            /// @brief @c true iff neither component is the INVALID sentinel.
            explicit operator bool() const noexcept {
                return !(x == INVALID || y == INVALID);
            }
        };

        /**
         * @brief Sparse, generation-tracked storage of payloads @c T over a fixed @c W x @c H cell array.
         *
         * Cells are addressed by @ref grid_coord or by a flat linear index (@c y*width+x). Payloads
         * live in a dense @c m_grid vector reached through the @c m_coords table; a free list recycles
         * vacated slots. Every @c set / @c clear bumps the cell's monotonic generation counter, so a
         * captured @c (index, generation) handle goes stale on overwrite/clear instead of aliasing a
         * new payload. Backs @ref neutrino::physics::grid.
         *
         * @tparam T The per-cell payload type.
         */
        template<typename T>
        class grid_storage {
            public:
                /// @brief Construct an empty @p w x @p h grid (all cells INVALID, generations 0).
                grid_storage(uint32_t w, uint32_t h)
                    : m_grid_with(w),
                      m_grid_height(h) {
                    m_coords.resize(m_grid_with * m_grid_height, grid_coord::INVALID);
                    m_generation.resize(m_grid_with * m_grid_height, 0u);
                }

                /**
                 * @brief Insert or overwrite the payload at cell @p c, constructing @c T in place.
                 * @tparam Args Constructor argument types for @c T.
                 * @param c    Target cell (must be in range; aborts otherwise).
                 * @param args Forwarded to @c T's constructor.
                 * @note Any set (insert OR overwrite) bumps the cell's generation, invalidating prior handles.
                 */
                template<typename... Args>
                void set(const grid_coord& c, Args&&... args) {
                    ENFORCE(is_valid(c));
                    const auto idx = index(c);
                    if (m_coords[idx] == grid_coord::INVALID) {
                        if (m_free_list.empty()) {
                            m_grid.emplace_back(std::forward <Args>(args)...);
                            m_coords[idx] = static_cast <uint32_t>(m_grid.size() - 1);
                        } else {
                            const auto pos = m_free_list.back();
                            m_free_list.pop_back();
                            m_grid[pos] = T(std::forward <Args>(args)...);
                            m_coords[idx] = pos;
                        }
                    } else {
                        m_grid[m_coords[idx]] = T(std::forward <Args>(args)...);
                    }
                    ++m_generation[idx]; // any set (insert or overwrite) invalidates prior handles
                }

                /**
                 * @brief Empty cell @p c (recycle its payload slot), bumping its generation.
                 * @param c Target cell; an out-of-range or already-empty cell is a tolerant no-op.
                 */
                void clear(const grid_coord& c) {
                    if (!is_valid(c)) {
                        return;
                    }
                    const auto idx = index(c);
                    if (m_coords[idx] != grid_coord::INVALID) {
                        m_free_list.push_back(m_coords[idx]);
                        m_coords[idx] = grid_coord::INVALID;
                        ++m_generation[idx];
                    }
                }

                /**
                 * @brief Empty every cell and drop all payloads (capacity retained).
                 * @note The per-cell generations are deliberately NOT reset -- keeping them monotonic
                 *       means a handle made before the reset can never alias a cell refilled after it.
                 */
                void reset() {
                    std::fill(m_coords.begin(), m_coords.end(), grid_coord::INVALID);
                    m_free_list.clear();
                    m_grid.clear();
                    // m_generation is deliberately NOT reset: keeping it monotonic means a handle
                    // made before the reset can never alias a cell refilled after it.
                }

                /**
                 * @brief Pointer to the payload at cell @p c, or @c nullptr if out of range or empty.
                 * @param c Target cell.
                 * @return A pointer into the dense store (stable only until the next set/clear/reset).
                 */
                [[nodiscard]] const T* get(const grid_coord& c) const noexcept {
                    if (!is_valid(c)) {
                        return nullptr;
                    }
                    const auto idx = index(c);
                    if (m_coords[idx] == grid_coord::INVALID) {
                        return nullptr;
                    }
                    return &m_grid[m_coords[idx]];
                }

                /// @copydoc get(const grid_coord&) const
                [[nodiscard]] T* get(const grid_coord& c) noexcept {
                    if (!is_valid(c)) {
                        return nullptr;
                    }
                    const auto idx = index(c);
                    if (m_coords[idx] == grid_coord::INVALID) {
                        return nullptr;
                    }
                    return &m_grid[m_coords[idx]];
                }

                /**
                 * @brief Is coord @p p a non-null, in-range cell?
                 * @param p Coord to test.
                 * @return @c true iff @p p is valid and within @c [0,width) x @c [0,height).
                 */
                [[nodiscard]] bool is_valid(const grid_coord& p) const noexcept {
                    if (!p) {
                        return false;
                    }
                    return (p.x < m_grid_with) && (p.y < m_grid_height);
                }

                // Linear (flat) cell addressing -- a stable handle into the dense m_coords array
                // (idx = y * width + x). Used by the grid owner to hold a cell by index without
                // round-tripping through physical coords. Tolerant: out-of-range / empty -> nullptr.

                /// @brief Total addressable cell count (@c width*height), the linear-index domain.
                [[nodiscard]] uint32_t cell_count() const noexcept {
                    return m_grid_with * m_grid_height;
                }

                /**
                 * @brief Payload at flat cell index @p idx (@c y*width+x), or @c nullptr if out of
                 *        range or empty.
                 * @param idx Linear cell index.
                 * @return A pointer into the dense store (stable only until the next set/clear/reset).
                 */
                [[nodiscard]] const T* get_linear(uint32_t idx) const noexcept {
                    if (idx >= m_coords.size() || m_coords[idx] == grid_coord::INVALID) {
                        return nullptr;
                    }
                    return &m_grid[m_coords[idx]];
                }

                /// @copydoc get_linear(uint32_t) const
                [[nodiscard]] T* get_linear(uint32_t idx) noexcept {
                    if (idx >= m_coords.size() || m_coords[idx] == grid_coord::INVALID) {
                        return nullptr;
                    }
                    return &m_grid[m_coords[idx]];
                }

                /**
                 * @brief Empty the cell at flat index @p idx, bumping its generation.
                 * @param idx Linear cell index; an out-of-range or empty cell is a tolerant no-op.
                 */
                void clear_linear(uint32_t idx) {
                    if (idx >= m_coords.size() || m_coords[idx] == grid_coord::INVALID) {
                        return;
                    }
                    m_free_list.push_back(m_coords[idx]);
                    m_coords[idx] = grid_coord::INVALID;
                    ++m_generation[idx];
                }

                /**
                 * @brief The mutation counter of cell @p idx -- bumped on every set/clear.
                 * @param idx Linear cell index.
                 * @return The current generation (out-of-range -> 0). A stable handle is current only
                 *         while this still matches the value captured when it was made. NOT reset by
                 *         @ref reset, so handles never alias across a reset.
                 */
                [[nodiscard]] uint32_t generation_linear(uint32_t idx) const noexcept {
                    return idx < m_generation.size() ? m_generation[idx] : 0u;
                }

                /// @brief The grid width in cells.
                [[nodiscard]] uint32_t get_width() const noexcept {
                    return m_grid_with;
                }

                /// @brief The grid height in cells.
                [[nodiscard]] uint32_t get_height() const noexcept {
                    return m_grid_height;
                }

            private:
                /// @brief Flat index (@c y*width+x) of coord @p p.
                [[nodiscard]] uint32_t index(const grid_coord& p) const noexcept {
                    return p.y * m_grid_with + p.x;
                }

            private:
                std::vector <T> m_grid;              ///< Dense payload store (indexed via m_coords).
                std::vector <uint32_t> m_coords;     ///< Per-cell slot index into m_grid, or INVALID if empty.
                std::vector <uint32_t> m_free_list;  ///< Recycled m_grid slots from cleared cells.
                std::vector <uint32_t> m_generation; ///< Per-cell mutation counter (handle staleness).

                uint32_t m_grid_with;                ///< Grid width in cells.
                uint32_t m_grid_height;              ///< Grid height in cells.
        };
    }

    /// @brief Test-only access to the grid's internal mapping. Defined only in the test TU.
    struct grid_test_access;

    /**
     * @brief A uniform 2D spatial grid over an axis-aligned physical box -- the static-tilemap
     *        broadphase behind @ref world.
     *
     * Divides its physical bounds into @c W x @c H equal cells, each holding at most one payload
     * @c T. Maps world coordinates to cells, stores them sparsely (see @ref detail::grid_storage),
     * and offers AABB (@ref query), swept (@ref swept), and ray (@ref raycast) traversals plus the
     * owner-facing stable cell handle API (@ref to_cell / @ref at / @ref cell_generation) and the
     * generic @ref compile_runs rectangle-merge pass.
     *
     * @tparam T The per-cell payload type (the world stores a tile record).
     * @see from_tile_size for tilemap-exact construction.
     */
    template<typename T>
    class grid {
        public:
            /// @brief Grants the test harness access to the private physical<->cell mapping.
            friend struct grid_test_access;

            /**
             * @brief Construct a @p w x @p h grid spanning the physical box @p grid_min .. @p grid_max.
             * @param w,h           Cell counts along x and y.
             * @param grid_min      World-space minimum corner of the grid's extent.
             * @param grid_max      World-space maximum corner; the cell size is the box divided by w/h.
             */
            grid(uint32_t w, uint32_t h, const vec& grid_min, const vec& grid_max)
                : m_grid(w, h),
                  m_physical_bounds(grid_min, grid_max),
                  m_cell_dim{
                      (grid_max.x() - grid_min.x()) / static_cast <float>(w),
                      (grid_max.y() - grid_min.y()) / static_cast <float>(h)
                  } {
            }

            /**
             * @brief Tilemap-friendly construction from an origin, a cell size, and a cell count.
             *
             * The physical extent is DERIVED (@c max = @p origin + count * @p tile_size), so the cell
             * size is exactly @p tile_size -- there is no separate, possibly-mismatched physical box.
             *
             * @param origin    World-space minimum corner (tile (0,0)'s corner).
             * @param tile_size Per-cell size.
             * @param cols,rows Cell counts along x and y.
             * @return The constructed grid.
             */
            [[nodiscard]] static grid from_tile_size(const vec& origin, const vec& tile_size,
                                                     uint32_t cols, uint32_t rows) {
                const vec grid_max{
                    origin.x() + static_cast <float>(cols) * tile_size.x(),
                    origin.y() + static_cast <float>(rows) * tile_size.y()
                };
                return grid(cols, rows, origin, grid_max);
            }

            /**
             * @brief Insert or overwrite the payload in the cell containing world point @p v.
             * @tparam Args Constructor argument types for @c T.
             * @param v    World point selecting the target cell (must be inside the grid).
             * @param args Forwarded to @c T's constructor.
             */
            template<typename... Args>
            void set(const vec& v, Args&&... args) {
                m_grid.set(physical_to_grid(v), std::forward <Args>(args)...);
            }

            /**
             * @brief Payload of the cell containing world point @p v, or @c nullptr if empty/outside.
             * @param v World point.
             */
            const T* get(const vec& v) const {
                return m_grid.get(physical_to_grid(v));
            }

            /// @copydoc get(const vec&) const
            T* get(const vec& v) {
                return m_grid.get(physical_to_grid(v));
            }

            /**
             * @brief Empty the cell containing world point @p v.
             * @param v World point; outside the grid is a tolerant no-op.
             */
            void clear(const vec& v) {
                m_grid.clear(physical_to_grid(v));
            }

            /// @brief Empty every cell (generations stay monotonic; see @ref detail::grid_storage::reset).
            void reset() {
                m_grid.reset();
            }

            // ---- owner-facing stable cell handle -----------------------------------------
            // The grid hides cell coords from queries, but its OWNER (the world) needs a stable
            // handle to a cell -- like a pool slot index for a body. to_cell(v) gives the linear
            // index of the cell containing v (INVALID_CELL if v is outside); at()/clear_at()
            // address a cell by that handle. Stable: the index is positional and never moves.

            /// @brief Sentinel returned by @ref to_cell for a point outside the grid.
            static constexpr uint32_t INVALID_CELL = std::numeric_limits <uint32_t>::max();

            /**
             * @brief The stable linear handle of the cell containing world point @p v.
             * @param v World point.
             * @return The cell's flat index (@c y*width+x), or @ref INVALID_CELL if @p v is outside
             *         the grid. The index is positional and never moves.
             */
            [[nodiscard]] uint32_t to_cell(const vec& v) const {
                const auto c = physical_to_grid(v);
                if (!c) {
                    return INVALID_CELL;
                }
                return c.y * m_grid.get_width() + c.x;
            }

            /**
             * @brief Payload of the cell with linear handle @p cell, or @c nullptr if empty/out of range.
             * @param cell Linear cell handle (e.g. from @ref to_cell).
             */
            [[nodiscard]] const T* at(uint32_t cell) const { return m_grid.get_linear(cell); }
            /// @copydoc at(uint32_t) const
            [[nodiscard]] T* at(uint32_t cell) { return m_grid.get_linear(cell); }
            /**
             * @brief Empty the cell with linear handle @p cell.
             * @param cell Linear cell handle; out of range / empty is a tolerant no-op.
             */
            void clear_at(uint32_t cell) { m_grid.clear_linear(cell); }

            /**
             * @brief The generation of cell @p cell -- bumped on every set/clear.
             * @param cell Linear cell handle.
             * @return The current generation. The owner stamps this into a handle at creation and
             *         re-checks it to detect a cell overwritten/refilled since.
             */
            [[nodiscard]] uint32_t cell_generation(uint32_t cell) const { return m_grid.generation_linear(cell); }

            /**
             * @brief World-space box of the cell with linear handle @p cell.
             * @param cell Linear cell handle.
             * @return The cell's AABB (for the owner's "does the payload fit its cell" checks).
             */
            [[nodiscard]] aabb cell_box_at(uint32_t cell) const {
                const uint32_t w = m_grid.get_width();
                return cell_box(detail::grid_coord{cell % w, cell / w});
            }

            /**
             * @brief Generic boundary-compile post-process: greedily merge maximal RECTANGLES of
             *        same-group occupied cells, emit each as a world-space AABB, and clear them.
             *
             * Pure spatial and semantics-free: the caller's @p same_group predicate encodes ALL
             * meaning. Each merged region is the union of its cell boxes; cells the caller never
             * groups are left in place. The owner turns each region into its own collider (e.g. a
             * merged BVH resident), so the grid keeps only the un-merged remainder -- removing the
             * internal tile seams that cause ghost-vertex snagging on long flat/sloped runs.
             *
             * @tparam SameGroup Predicate @c same_group(const T& seed, const T& cell, const aabb&
             *                   cell_box) -> bool: @c true iff @c cell is mergeable AND belongs to
             *                   @c seed's group. A cell is a rectangle seed iff
             *                   @c same_group(cell, cell, cell_box) holds (e.g. "a solid block tile
             *                   filling its cell, same material/filter, opted-in").
             * @tparam OnRun     Callback @c on_run(const aabb& region, const T& sample) -> void,
             *                   invoked once per merged rectangle with a sample payload.
             * @param same_group The grouping predicate.
             * @param on_run     Per-merged-rectangle sink.
             * @warning Destructive: the merged source cells are cleared from the grid (their per-cell
             *          handles go stale).
             */
            template<class SameGroup, class OnRun>
            void compile_runs(SameGroup&& same_group, OnRun&& on_run) {
                const uint32_t w = m_grid.get_width();
                const uint32_t h = m_grid.get_height();
                std::vector <char> consumed(static_cast <std::size_t>(w) * h, 0);
                const auto idx = [w](uint32_t x, uint32_t y) { return y * w + x; };
                const auto box = [this](uint32_t x, uint32_t y) {
                    return cell_box(detail::grid_coord{x, y});
                };
                // payload at (x,y) iff occupied, not yet consumed, and same group as `seed` (or, when
                // seed == nullptr, mergeable on its own -> a valid rectangle seed). Returns nullptr otherwise.
                const auto match = [&](uint32_t x, uint32_t y, const T* seed) -> const T* {
                    const uint32_t c = idx(x, y);
                    if (consumed[c]) {
                        return nullptr;
                    }
                    const T* p = m_grid.get_linear(c);
                    if (!p) {
                        return nullptr;
                    }
                    return same_group(seed ? *seed : *p, *p, box(x, y)) ? p : nullptr;
                };

                for (uint32_t y = 0; y < h; ++y) {
                    for (uint32_t x = 0; x < w; ++x) {
                        const T* seed = match(x, y, nullptr);
                        if (!seed) {
                            continue;
                        }
                        const T sample = *seed; // copy before any clear invalidates the pointer
                        uint32_t x2 = x;
                        while (x2 + 1 < w && match(x2 + 1, y, &sample)) {
                            ++x2;
                        }
                        const auto row_ok = [&](uint32_t ry) {
                            for (uint32_t cx = x; cx <= x2; ++cx) {
                                if (!match(cx, ry, &sample)) {
                                    return false;
                                }
                            }
                            return true;
                        };
                        uint32_t y2 = y;
                        while (y2 + 1 < h && row_ok(y2 + 1)) {
                            ++y2;
                        }
                        const aabb region = aabb::combine(box(x, y), box(x2, y2));
                        for (uint32_t ry = y; ry <= y2; ++ry) {
                            for (uint32_t cx = x; cx <= x2; ++cx) {
                                consumed[idx(cx, ry)] = 1;
                                m_grid.clear_linear(idx(cx, ry));
                            }
                        }
                        on_run(region, sample);
                    }
                }
            }

            /**
             * @brief Visit every occupied cell overlapping the AABB @p region.
             * @tparam Fn Callback @c (const T&, const aabb& cell_box). A @c void callback visits every
             *            occupied cell; a @c bool callback returns @c false to stop the walk early.
             * @param region   The world-space query box (clipped to the grid extent).
             * @param callback Per-cell sink.
             */
            template<typename Fn>
            void query(const aabb& region, Fn&& callback) const {
                if (!intersects(region, m_physical_bounds)) {
                    return;
                }
                const auto min_x = std::max(m_physical_bounds.min.x(), region.min.x());
                const auto max_x = std::min(m_physical_bounds.max.x(), region.max.x());

                const auto min_y = std::max(m_physical_bounds.min.y(), region.min.y());
                const auto max_y = std::min(m_physical_bounds.max.y(), region.max.y());

                const auto min_c = physical_to_grid(vec{min_x, min_y});
                const auto max_c = physical_to_grid(vec{max_x, max_y});

                for (uint32_t y = min_c.y; y <= max_c.y; ++y) {
                    for (uint32_t x = min_c.x; x <= max_c.x; ++x) {
                        const detail::grid_coord c{x, y};
                        if (const auto* e = m_grid.get(c)) {
                            // Same void-or-bool callback contract as raycast: a void callback
                            // visits every occupied cell; a bool callback returns false to stop.
                            if constexpr (std::is_void_v <std::invoke_result_t <Fn&, const T&, const aabb&>>) {
                                callback(*e, cell_box(c));
                            } else {
                                if (!callback(*e, cell_box(c))) {
                                    return;
                                }
                            }
                        }
                    }
                }
            }

            /**
             * @brief Swept broadphase: visit the occupied cells a moving shape's bound can overlap as
             *        it travels @p start_bound -> @p start_bound + @p delta over a frame.
             *
             * The swept band is the cell rectangle of the union of the bound at both ends, covering
             * the whole path (cell-level anti-tunnelling for per-frame moves). Pure broadphase -- the
             * world narrow-phases (swept) each cell and keeps the earliest TOI.
             *
             * @tparam Fn Same callback contract as @ref query: @c (const T&, const aabb&), void-or-bool.
             * @param start_bound The shape's enclosing AABB at the start (a circle sweeps via its box).
             * @param delta       The frame displacement.
             * @param callback    Per-cell sink.
             */
            template<typename Fn>
            void swept(const aabb& start_bound, const vec& delta, Fn&& callback) const {
                const aabb band = aabb::combine(start_bound, translate(start_bound, delta));
                query(band, std::forward<Fn>(callback));
            }

            /**
             * @brief Visit the occupied cells the segment @p from -> @p to crosses, near-to-far, via
             *        an Amanatides-Woo DDA.
             *
             * Handles non-unit / rectangular cells and axis-aligned rays, with supercover treatment
             * of exact gridline and cell-corner crossings (so no diagonal corner tunnelling). An
             * out-of-grid ray is a tolerant no-op (clipped away first).
             *
             * @tparam Fn Callback @c (const T&, const aabb& cell_box, float t). @c t is the entry
             *            parameter along the ORIGINAL @p from -> @p to ray (in [0,1]), comparable with
             *            the BVH raycast for a grid+BVH merge. A @c void callback visits every crossed
             *            occupied cell; a @c bool callback returns @c false to stop early.
             * @param from     Ray origin.
             * @param to       Ray endpoint.
             * @param callback Per-cell sink.
             */
            template<typename Fn>
            void raycast(const vec& from, const vec& to, Fn&& callback) const {
                const auto clipped = clip(m_physical_bounds, segment{from, to});
                if (!clipped) {
                    return; // the ray misses the grid entirely
                }
                const vec cf = clipped->from;
                const vec ct = clipped->to;

                const float dirx = to.x() - from.x();
                const float diry = to.y() - from.y();
                const float len2 = dirx * dirx + diry * diry;
                if (len2 < constants::POINT_EPS * constants::POINT_EPS) {
                    visit_cell(physical_to_grid(cf), 0.0f, callback); // degenerate: use the clipped point
                    return;
                }

                // entry/exit parameters of the clipped portion, measured along the ORIGINAL ray.
                const float t0 = ((cf.x() - from.x()) * dirx + (cf.y() - from.y()) * diry) / len2;
                const float t1 = ((ct.x() - from.x()) * dirx + (ct.y() - from.y()) * diry) / len2;

                const detail::grid_coord start = physical_to_grid(cf);
                int cx = static_cast <int>(start.x);
                int cy = static_cast <int>(start.y);

                const float eps = constants::POINT_EPS; // world-distance tol (direction != 0 test)
                const float inf = std::numeric_limits <float>::infinity();
                // tMax / t1 live in NORMALIZED ray-parameter space (t along from->to), so a world
                // tolerance must be converted: a world distance POINT_EPS spans POINT_EPS/|ray| in
                // t. Using a world eps here would merge far-apart crossings on a long ray into a
                // false "corner" -- emitting cells the ray never touches.
                const float t_eps = constants::POINT_EPS / std::sqrt(len2);

                // Directional start on an exact internal boundary. floor (physical_to_grid) maps a
                // boundary to the upper/right cell, but a ray moving in the negative direction
                // enters the lower/left cell -- pick the cell with positive-length forward overlap,
                // so a wall BEHIND the origin (opposite travel) is never emitted as a spurious hit.
                // (dir == 0 keeps the floored cell; the shadow row/col below covers both sides.)
                if (dirx < -eps) {
                    const float fx = (cf.x() - m_physical_bounds.min.x()) / m_cell_dim.x();
                    const float kx = std::round(fx);
                    if (std::abs(fx - kx) < constants::POINT_EPS / m_cell_dim.x() && kx >= 1.0f
                        && kx <= static_cast <float>(m_grid.get_width() - 1)) {
                        cx = static_cast <int>(kx) - 1;
                    }
                }
                if (diry < -eps) {
                    const float fy = (cf.y() - m_physical_bounds.min.y()) / m_cell_dim.y();
                    const float ky = std::round(fy);
                    if (std::abs(fy - ky) < constants::POINT_EPS / m_cell_dim.y() && ky >= 1.0f
                        && ky <= static_cast <float>(m_grid.get_height() - 1)) {
                        cy = static_cast <int>(ky) - 1;
                    }
                }

                // Per axis: step direction, t to cross one cell (tDelta), and t of the first cell
                // boundary ahead (tMax). dir==0 -> never step that axis (handles axis-aligned rays).
                int stepX = 0, stepY = 0;
                float tDeltaX = inf, tDeltaY = inf, tMaxX = inf, tMaxY = inf;
                if (std::abs(dirx) > eps) {
                    stepX = dirx > 0 ? 1 : -1;
                    tDeltaX = m_cell_dim.x() / std::abs(dirx);
                    const float bx = m_physical_bounds.min.x()
                                     + (static_cast <float>(cx) + (stepX > 0 ? 1.0f : 0.0f)) * m_cell_dim.x();
                    tMaxX = (bx - from.x()) / dirx;
                }
                if (std::abs(diry) > eps) {
                    stepY = diry > 0 ? 1 : -1;
                    tDeltaY = m_cell_dim.y() / std::abs(diry);
                    const float by = m_physical_bounds.min.y()
                                     + (static_cast <float>(cy) + (stepY > 0 ? 1.0f : 0.0f)) * m_cell_dim.y();
                    tMaxY = (by - from.y()) / diry;
                }

                // Supercover for the degenerate "on a grid line" case: an axis-aligned ray lying
                // EXACTLY on an internal grid line touches the cells on BOTH sides along its whole
                // length, but the floor mapping only assigns one side. Track the other ("shadow")
                // row/column so it is visited too. (Only possible for an axis-aligned ray.)
                int shadowX = -1, shadowY = -1;
                if (stepX == 0) {
                    const float fx = (from.x() - m_physical_bounds.min.x()) / m_cell_dim.x();
                    // fx is in CELL units; scale the world tolerance into cell space.
                    if (const float kx = std::round(fx);
                        std::abs(fx - kx) < constants::POINT_EPS / m_cell_dim.x() && kx >= 1.0f
                        && kx <= static_cast <float>(m_grid.get_width() - 1)) {
                        shadowX = static_cast <int>(kx) - 1;
                    }
                }
                if (stepY == 0) {
                    const float fy = (from.y() - m_physical_bounds.min.y()) / m_cell_dim.y();
                    // fy is in CELL units; scale the world tolerance into cell space.
                    if (const float ky = std::round(fy);
                        std::abs(fy - ky) < constants::POINT_EPS / m_cell_dim.y() && ky >= 1.0f
                        && ky <= static_cast <float>(m_grid.get_height() - 1)) {
                        shadowY = static_cast <int>(ky) - 1;
                    }
                }

                // Visit cell (x,y) and, for an on-gridline ray, its shadow neighbour. Out-of-range
                // cells are skipped (visit_cell is tolerant). Returns false if a bool callback stops.
                const auto emit = [&](int x, int y, float t) -> bool {
                    if (x >= 0 && y >= 0
                        && !visit_cell(detail::grid_coord{static_cast <uint32_t>(x), static_cast <uint32_t>(y)}, t,
                                       callback)) {
                        return false;
                    }
                    if (shadowY >= 0 && x >= 0
                        && !visit_cell(detail::grid_coord{static_cast <uint32_t>(x), static_cast <uint32_t>(shadowY)},
                                       t, callback)) {
                        return false;
                    }
                    if (shadowX >= 0 && y >= 0
                        && !visit_cell(detail::grid_coord{static_cast <uint32_t>(shadowX), static_cast <uint32_t>(y)},
                                       t, callback)) {
                        return false;
                    }
                    return true;
                };

                // Visit the start cell (entry t = t0), then step boundary-to-boundary.
                if (!emit(cx, cy, std::max(0.0f, t0))) {
                    return;
                }
                while (true) {
                    if (stepX != 0 && stepY != 0 && std::abs(tMaxX - tMaxY) <= t_eps) {
                        // Exact cell-CORNER crossing: the ray touches BOTH side-adjacent cells, not
                        // just the diagonal one -- visit both (supercover, no corner tunnelling).
                        const float t_cross = tMaxX; // == tMaxY
                        if (t_cross > t1 + t_eps) {
                            break;
                        }
                        if (!emit(cx + stepX, cy, t_cross)) return;
                        if (!emit(cx, cy + stepY, t_cross)) return;
                        cx += stepX;
                        cy += stepY;
                        tMaxX += tDeltaX;
                        tMaxY += tDeltaY;
                        if (cx < 0 || cy < 0) break;
                        if (!emit(cx, cy, t_cross)) return;
                    } else {
                        float t_cross;
                        if (tMaxX < tMaxY) {
                            cx += stepX;
                            t_cross = tMaxX;
                            tMaxX += tDeltaX;
                        } else {
                            cy += stepY;
                            t_cross = tMaxY;
                            tMaxY += tDeltaY;
                        }
                        if (t_cross > t1 + t_eps || cx < 0 || cy < 0) {
                            break; // past the clipped exit / stepped off the grid
                        }
                        if (!emit(cx, cy, t_cross)) return;
                    }
                }
            }

        private:
            /**
             * @brief Map world point @p v to its cell coord.
             * @param v World point.
             * @return The containing cell's coord, or the INVALID coord if @p v is outside the grid.
             */
            [[nodiscard]] detail::grid_coord physical_to_grid(const vec& v) const {
                if (!contains(m_physical_bounds, v)) {
                    return {};
                }
                const vec p = (v - m_physical_bounds.min) / m_cell_dim;
                const auto cx = std::min(static_cast <uint32_t>(p.x()), m_grid.get_width() - 1u);
                const auto cy = std::min(static_cast <uint32_t>(p.y()), m_grid.get_height() - 1u);
                return {cx, cy};
            }

            /**
             * @brief World-space box of cell coord @p c.
             * @param c A valid cell coord (aborts if INVALID).
             * @return The cell's AABB in world space.
             */
            [[nodiscard]] aabb cell_box(const detail::grid_coord& c) const {
                ENFORCE(c);
                const vec p{
                    static_cast <float>(c.x) * m_cell_dim.x(),
                    static_cast <float>(c.y) * m_cell_dim.y()
                };
                const vec corner = p + m_physical_bounds.min;
                return {corner, corner + m_cell_dim};
            }

            /**
             * @brief Report cell @p c at entry parameter @p t to the raycast @p callback.
             * @tparam Fn The raycast callback @c (const T&, const aabb&, float).
             * @param c        Cell coord; empty / out-of-range cells are skipped.
             * @param t        Entry parameter along the ray.
             * @param callback The per-cell sink.
             * @return @c false only when a bool-returning callback asks to stop; a void callback
             *         always returns @c true (continue).
             */
            template<typename Fn>
            bool visit_cell(const detail::grid_coord& c, float t, Fn&& callback) const {
                const T* e = m_grid.get(c); // tolerant: nullptr if empty or out of range
                if (!e) {
                    return true;
                }
                if constexpr (std::is_void_v <std::invoke_result_t <Fn&, const T&, const aabb&, float>>) {
                    callback(*e, cell_box(c), t);
                    return true;
                } else {
                    return callback(*e, cell_box(c), t); // bool callback: false stops the walk
                }
            }

        private:
            detail::grid_storage <T> m_grid; ///< Sparse per-cell payload storage + generations.
            aabb m_physical_bounds;          ///< World-space extent the grid tiles.
            const vec m_cell_dim;            ///< Per-cell size (extent / cell count along each axis).
    };
}
