//
// Created by igor on 08/07/2026.
//

#include <neutrino/video/world/world_renderer.hh>
#include <neutrino/video/draw.hh>

#include <algorithm>
#include <cstdlib>
#include <utility>
#include <failsafe/enforce.hh>
#include "services/service_locator.hh"

namespace neutrino {
    namespace {
        // The application-wide cache published at ready time. Fails loudly rather
        // than acquiring against a null cache if used before the app is ready.
        resource_cache& located_cache() {
            resource_cache* cache = service_locator::instance().get_resource_cache();
            ENFORCE(cache != nullptr)
                ("world_renderer: no resource_cache is available; is the application ready?");
            return *cache;
        }

        // Conservative cull inflation, in cells, for tiles that draw larger than the
        // map cell (or with a draw offset) and so overhang their grid rect. All four
        // sides are inflated by this amount because a draw offset can push a tile any
        // direction; over-including a border cell is harmless (off-screen tiles clip),
        // whereas under-including would pop a partially visible edge tile.
        int compute_overhang_cells(const world& w) {
            const int mtw  = static_cast <int>(w.tile_width());
            const int mth  = static_cast <int>(w.tile_height());
            const int cell = std::min(mtw, mth);
            if (cell <= 0) {
                return 0;
            }
            int overhang_px = 0;
            for (const world_tileset& ts : w.tilesets()) {
                int tile_w = static_cast <int>(ts.tile_width);
                int tile_h = static_cast <int>(ts.tile_height);
                for (const world_tile& t : ts.tiles) {
                    if (t.image) {
                        // A sub-rect (image-collection atlas) is the tile's real size;
                        // otherwise the tile is its whole image.
                        const int tw = t.source_rect ? t.source_rect->w : static_cast <int>(t.image->width);
                        const int th = t.source_rect ? t.source_rect->h : static_cast <int>(t.image->height);
                        tile_w = std::max(tile_w, tw);
                        tile_h = std::max(tile_h, th);
                    }
                }
                const int over_x = std::max(0, tile_w - mtw) + std::abs(ts.offset_x);
                const int over_y = std::max(0, tile_h - mth) + std::abs(ts.offset_y);
                overhang_px = std::max({overhang_px, over_x, over_y});
            }
            return (overhang_px + cell - 1) / cell; // ceil
        }

        // Wrap an image layer's picture as a collection tileset so it flows through
        // build_bundle/the cache unchanged, content-keyed so identical pictures across
        // levels share a bundle and survive switches. A static layer is one tile (the
        // whole image); an animated layer is one tile per frame, with tile 0 carrying an
        // animation over them all -- so the bundle exposes state(0), exactly like an
        // animated tile cell.
        world_tileset image_layer_as_tileset(const world_image_layer& layer) {
            world_tileset ts;
            ts.first_gid = 1;
            if (layer.frames.empty()) {
                ts.tile_count = 1;
                world_tile t;
                t.id = 0;
                t.image = *layer.image;
                ts.tiles.push_back(std::move(t));
                return ts;
            }
            ts.tile_count = static_cast <unsigned>(layer.frames.size());
            ts.tiles.reserve(layer.frames.size());
            for (std::size_t i = 0; i < layer.frames.size(); ++i) {
                world_tile t;
                t.id = static_cast <world_local_tile_id>(i);
                t.image = layer.frames[i].image;
                ts.tiles.push_back(std::move(t));
            }
            for (std::size_t i = 0; i < layer.frames.size(); ++i) {
                ts.tiles[0].animation.push_back(world_tile_animation_frame{
                    static_cast <world_local_tile_id>(i), layer.frames[i].duration});
            }
            return ts;
        }

        // The declared height of an image layer's picture (first frame for an animated
        // one), used as a fallback. 0 for an empty layer.
        unsigned image_layer_height(const world_image_layer& layer) {
            if (!layer.frames.empty()) {
                return layer.frames.front().image.height;
            }
            return layer.image ? layer.image->height : 0u;
        }

        // The visual an image layer currently shows, plus that frame's pixel height. For
        // an animated layer the resolved visual is matched back to its frame so the anchor
        // uses THAT frame's height -- otherwise frames of different heights would shift the
        // layer's top-left as the animation advances (the visual origin is baked from each
        // frame's own height).
        std::pair <sprite_visual_ref, unsigned> current_image_visual(
                const world_image_layer& layer, const bundle_handle& handle) {
            if (handle.state(0).valid()) {
                const sprite_visual_ref v = sprite_state_appearance(handle.state(0)).visual;
                for (std::size_t i = 0; i < layer.frames.size(); ++i) {
                    if (handle.visual(static_cast <world_local_tile_id>(i)) == v) {
                        return {v, layer.frames[i].image.height};
                    }
                }
                return {v, image_layer_height(layer)};
            }
            return {handle.visual(0), image_layer_height(layer)};
        }
    } // namespace

    world_renderer::world_renderer(const world& w)
        : world_renderer(w, located_cache()) {
    }

    world_renderer::world_renderer(const world& w, resource_cache& cache)
        : m_world(&w), m_cache(&cache) {
        const auto& tilesets = w.tilesets();
        m_handles.reserve(tilesets.size());
        // Acquire every tileset's bundle; a shared one simply bumps its refcount.
        // If a later tileset fails to build, the destructor will not run for this
        // partially constructed object, so release what we already hold before the
        // exception escapes -- otherwise those refcounts leak for a renderer that
        // never exists.
        try {
            for (const world_tileset& ts : tilesets) {
                m_handles.push_back(cache.acquire(ts));
            }
            // Each image layer's picture (static or animated) becomes its own
            // content-keyed bundle.
            for (const world_image_layer* il : w.image_layers()) {
                if (il->image || !il->frames.empty()) {
                    m_image_handles.emplace(il, cache.acquire(image_layer_as_tileset(*il)));
                }
            }
        } catch (...) {
            release_all();
            throw;
        }
        m_overhang_cells = compute_overhang_cells(w);
    }

    world_renderer::~world_renderer() {
        release_all();
    }

    world_renderer::world_renderer(world_renderer&& other) noexcept
        : m_world(other.m_world),
          m_cache(other.m_cache),
          m_handles(std::move(other.m_handles)),
          m_overhang_cells(other.m_overhang_cells),
          m_image_handles(std::move(other.m_image_handles)) {
        // Leave the source inert so its destructor releases nothing.
        other.m_handles.clear();
        other.m_image_handles.clear();
        other.m_world = nullptr;
        other.m_cache = nullptr;
        other.m_overhang_cells = 0;
    }

    world_renderer& world_renderer::operator=(world_renderer&& other) noexcept {
        if (this != &other) {
            release_all(); // drop our current handles before adopting the source's
            m_world = other.m_world;
            m_cache = other.m_cache;
            m_handles = std::move(other.m_handles);
            m_overhang_cells = other.m_overhang_cells;
            m_image_handles = std::move(other.m_image_handles);
            other.m_handles.clear();
            other.m_image_handles.clear();
            other.m_world = nullptr;
            other.m_cache = nullptr;
            other.m_overhang_cells = 0;
        }
        return *this;
    }

    void world_renderer::switch_to(const world& next) {
        ENFORCE(m_cache != nullptr)("world_renderer: switch_to on a moved-from renderer");
        // The temporary acquires every bundle of `next` before this move-assignment
        // releases the current level's, so a tileset shared across the switch never
        // drops to refcount zero (and is never rebuilt).
        *this = world_renderer(next, *m_cache);
    }

    const bundle_handle* world_renderer::image_layer_handle(const world_image_layer& layer) const noexcept {
        const auto it = m_image_handles.find(&layer);
        return it == m_image_handles.end() ? nullptr : &it->second;
    }

    world_point world_renderer::parallax_rest() const noexcept {
        return m_world ? m_world->parallax_origin() : world_point{0.0f, 0.0f};
    }

    unsigned world_renderer::image_layer_current_height(const world_image_layer& layer) const {
        const bundle_handle* handle = image_layer_handle(layer);
        return handle == nullptr ? 0u : current_image_visual(layer, *handle).second;
    }

    draw_stats world_renderer::draw(const camera& cam, const rect& viewport) {
        return draw(cam, viewport, {});
    }

    draw_stats world_renderer::draw(const camera& cam, const rect& viewport,
                                    const std::function <void(world_layer_id)>& after_layer) {
        ENFORCE(m_world);
        // Inject the map's parallax rest point so parallax layers align as authored;
        // the caller's camera doesn't need to carry it.
        camera view = cam;
        view.parallax_rest = m_world->parallax_origin();
        draw_stats stats;
        for (const auto& layer : m_world->layers()) {
            std::visit([&](const auto& concrete_layer) {
                draw_layer(concrete_layer, view, viewport, stats);
            }, layer);
            if (after_layer) {
                const world_layer_id id = std::visit(
                    [](const world_layer_header& header) { return header.id; }, layer);
                after_layer(id);
            }
        }
        return stats;
    }

    void world_renderer::draw_gid_at(world_tile_id gid, const world_point& anchor, sprite_flip flip,
                                     float rotation, const world_layer_header& layer, const camera& cam,
                                     const rect& viewport, draw_stats& stats) const {
        const world_tileset* ts = m_world->tileset_for(gid);
        if (ts == nullptr) {
            ++stats.skipped;
            return;
        }
        // The handle table is built in tilesets() order (ctor), so the tileset's index
        // into that vector indexes the aligned handle table.
        const std::size_t index = static_cast <std::size_t>(ts - m_world->tilesets().data());
        const bundle_handle& handle = handle_for_tileset(index);
        const world_local_tile_id local = ts->to_local(gid);
        sprite_visual_ref visual = handle.visual(local);
        if (!visual.valid()) {
            ++stats.skipped;
            return;
        }
        // Animated tile: the current frame from the shared clock (every instance in
        // lockstep); else the static visual. Flip composes on top either way.
        if (const sprite_state_id anim = handle.state(local); anim.valid()) {
            visual = sprite_state_appearance(anim).visual;
            if (!visual.valid()) {
                ++stats.skipped;
                return;
            }
        }

        const dim vp = viewport.dimensions();

        // Rotated or diagonally-flipped tiles, and non-orthogonal maps, keep the
        // anchor/scale path: the seam-free AABB tiling below assumes an axis-aligned
        // orthogonal grid. Hex/iso tiles overlap on a staggered layout, so they draw at
        // their anchor. All are rare or not seam-sensitive (hex tiles have transparent
        // edges that hide any 1px boundary). `unknown` is treated as orthogonal here, as
        // it is everywhere else (a direct-built map that never set an orientation).
        const world_orientation orient = m_world->orientation();
        const bool grid_aligned = orient == world_orientation::orthogonal
                                  || orient == world_orientation::unknown;
        if (rotation != 0.0f || (flip & sprite_flip::diagonal) == sprite_flip::diagonal
            || !grid_aligned) {
            const point sp = to_screen(cam, layer, vp, anchor);
            const point pos{viewport.x + sp.x, viewport.y + sp.y};
            const auto result = draw_sprite(pos, visual, {cam.zoom, flip, rotation});
            if (result.has_value()) {
                ++stats.drawn;
            } else {
                ++stats.failed;
            }
            return;
        }

        // Seam-free tiling: build the destination from two rounded world corners of the
        // tile's pixel AABB (bottom-left anchor + tileset offset, D1). to_screen rounds
        // each corner, so a shared world edge maps to one screen pixel and adjacent
        // tiles/objects meet exactly -- no 1px gap under fractional zoom.
        const tile_drawable d = ts->drawable(local);
        const world_point tl{anchor.x + static_cast <float>(d.origin.x),
                             anchor.y - static_cast <float>(d.src.h) + static_cast <float>(d.origin.y)};
        const world_point br{tl.x + static_cast <float>(d.src.w), tl.y + static_cast <float>(d.src.h)};
        const point s_tl = to_screen(cam, layer, vp, tl);
        const point s_br = to_screen(cam, layer, vp, br);
        const rect dst{viewport.x + s_tl.x, viewport.y + s_tl.y, s_br.x - s_tl.x, s_br.y - s_tl.y};

        if (draw_sprite(dst, visual, flip).has_value()) {
            ++stats.drawn;
        } else {
            ++stats.failed;
        }
    }

    void world_renderer::draw_cell(const world_tile_cell& cell, int cx, int cy,
                                   const world_layer_header& layer, const camera& cam,
                                   const rect& viewport, draw_stats& stats) const {
        if (cell.empty()) {
            return; // absent cell -- not a skip
        }
        // Cell -> world bottom-left anchor, honoring the map orientation (hex staggers).
        const world_point anchor = cell_to_world(*m_world, cx, cy);
        draw_gid_at(cell.gid, anchor, cell.flip, cell.rotation_degrees, layer, cam, viewport, stats);
    }

    void world_renderer::draw_layer(const world_tile_layer& layer, const camera& cam,
                                    const rect& viewport, draw_stats& stats) const {
        if (!layer.visible) {
            return;
        }

        // Infinite/chunked layers store cells in chunks, not the flat grid. Cull to
        // the chunks the (overhang-inflated) unclamped viewport rect intersects and
        // draw the visible cells of each at their world coordinates.
        if (!layer.chunks.empty()) {
            cell_range bounds = visible_cell_bounds(*m_world, layer, cam, viewport.dimensions());
            bounds.x0 -= m_overhang_cells;
            bounds.y0 -= m_overhang_cells;
            bounds.x1 += m_overhang_cells;
            bounds.y1 += m_overhang_cells;
            if (bounds.empty()) {
                return;
            }
            for (const world_tile_chunk& chunk : layer.chunks) {
                if (chunk.width <= 0 || chunk.height <= 0
                    || chunk.cells.size() < static_cast <std::size_t>(chunk.width) * chunk.height) {
                    continue; // malformed chunk -- skip rather than index out of range
                }
                // Intersect the chunk's cell rect with the visible bounds; the loops
                // run zero times for a non-intersecting chunk.
                const int cx0 = std::max(bounds.x0, chunk.x);
                const int cy0 = std::max(bounds.y0, chunk.y);
                const int cx1 = std::min(bounds.x1, chunk.x + chunk.width);
                const int cy1 = std::min(bounds.y1, chunk.y + chunk.height);
                for (int cy = cy0; cy < cy1; cy++) {
                    for (int cx = cx0; cx < cx1; cx++) {
                        const std::size_t idx = static_cast <std::size_t>(cy - chunk.y) * chunk.width
                                              + static_cast <std::size_t>(cx - chunk.x);
                        draw_cell(chunk.cells[idx], cx, cy, layer, cam, viewport, stats);
                    }
                }
            }
            return;
        }

        auto range = visible_cell_range(*m_world, layer, cam, viewport.dimensions());
        // Inflate for oversized-tile / offset overhang, then re-clamp to the layer.
        const int lw = static_cast <int>(layer.width);
        const int lh = static_cast <int>(layer.height);
        range.x0 = std::max(0, range.x0 - m_overhang_cells);
        range.y0 = std::max(0, range.y0 - m_overhang_cells);
        range.x1 = std::min(lw, range.x1 + m_overhang_cells);
        range.y1 = std::min(lh, range.y1 + m_overhang_cells);
        if (range.empty()) {
            return;
        }
        // Honor the map's render order so overlapping/oversized tiles paint in the
        // right z-order: right_* draws columns left->right, left_* right->left;
        // *_down draws rows top->bottom, *_up bottom->top. y is the outer (row) loop.
        const world_render_order order = m_world->render_order();
        const bool x_ascending = order == world_render_order::right_down
                              || order == world_render_order::right_up;
        const bool y_ascending = order == world_render_order::right_down
                              || order == world_render_order::left_down;
        for (int yi = 0, yn = range.y1 - range.y0; yi < yn; yi++) {
            const int y = y_ascending ? range.y0 + yi : range.y1 - 1 - yi;
            for (int xi = 0, xn = range.x1 - range.x0; xi < xn; xi++) {
                const int x = x_ascending ? range.x0 + xi : range.x1 - 1 - xi;
                draw_cell(layer.at(static_cast <unsigned>(x), static_cast <unsigned>(y)),
                          x, y, layer, cam, viewport, stats);
            }
        }
    }

    void world_renderer::draw_layer(const world_image_layer& layer, const camera& cam,
                                    const rect& viewport, draw_stats& stats) const {
        if (!layer.visible || (!layer.image && layer.frames.empty())) {
            return;
        }
        const auto it = m_image_handles.find(&layer);
        if (it == m_image_handles.end()) {
            ++stats.skipped;
            return;
        }
        // Animated image layers resolve the current frame off the shared clock (state 0),
        // like an animated tile cell; static ones use the single visual. The height is the
        // *current* frame's, so mixed-height frames keep a common top-left.
        const bundle_handle& handle = it->second;
        const auto [visual, height] = current_image_visual(layer, handle);
        if (!visual.valid()) {
            ++stats.skipped;
            return;
        }
        // The image's bottom-left sits at world (0, frame height) in the layer's own space;
        // the bundle's baked bottom-left origin then lands the image top-left at the layer
        // origin. Parallax/offset ride through to_screen.
        const world_point anchor{0.0f, static_cast <float>(height)};
        const point sp = to_screen(cam, layer, viewport.dimensions(), anchor);
        const point pos{viewport.x + sp.x, viewport.y + sp.y};
        if (draw_sprite(pos, visual, {cam.zoom}).has_value()) {
            ++stats.drawn;
        } else {
            ++stats.failed;
        }
    }

    void world_renderer::draw_layer(const world_object_layer& layer, const camera& cam,
                                    const rect& viewport, draw_stats& stats) const {
        if (!layer.visible) {
            return;
        }
        for (const world_object& object : layer.objects) {
            std::visit([&](const auto& obj) {
                const world_object_base& base = obj;
                // Only tile objects (gid != 0) are sprite-rendered; shapes/text and
                // hidden objects are handled elsewhere and are not counted.
                if (!base.visible || base.gid == 0) {
                    return;
                }
                // Tiled anchors a tile object by its bottom-left at (x,y) = origin and
                // rotates about that point -- matching the bundle's bottom-left visual
                // origin. draw_gid_at gives objects the same seam-free tiling as cells, so
                // side-by-side background bands don't show 1px gaps under fractional zoom.
                // Object width/height resize is deferred (needs a per-axis scale); tiles
                // draw at native size scaled by zoom.
                draw_gid_at(base.gid, base.origin, base.flip, static_cast <float>(base.rotation),
                            layer, cam, viewport, stats);
            }, object);
        }
    }

    void world_renderer::release_all() noexcept {
        if (m_cache != nullptr) {
            for (const bundle_handle& handle : m_handles) {
                m_cache->release(handle);
            }
            for (const auto& [layer, handle] : m_image_handles) {
                m_cache->release(handle);
            }
        }
        m_handles.clear();
        m_image_handles.clear();
    }
}
