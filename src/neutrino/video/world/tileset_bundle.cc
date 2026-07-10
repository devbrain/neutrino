//
// Created by igor on 07/07/2026.
//

#include <neutrino/video/world/tileset_bundle.hh>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <deque>
#include <optional>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>

#include <failsafe/enforce.hh>

#include <neutrino/video/image_loader.hh>
#include <neutrino/video/sprites.hh>
#include <neutrino/video/sprite/atlas_packer.hh>

namespace neutrino {
    namespace {
        // Gutter reserved on every side of each tile, so adjacent tiles do not bleed
        // into one another under linear filtering / non-integer scale.
        constexpr int tile_gutter = 1;

        [[nodiscard]] sdlpp::surface decode_world_image(const world_image& img) {
            if (!img.data.empty()) {
                return load_image(std::span <const std::uint8_t>(img.data.data(), img.data.size()));
            }
            return load_image(img.source);
        }

        // One packable tile: which source rectangle to pack, plus the local id and
        // draw origin to carry onto the resulting visual.
        struct tile_region {
            world_local_tile_id local;
            point               origin;
        };

        // Pack the gathered tile regions into atlas pages in a canonical RGBA format.
        // Source images may be palettized (indexed) -- e.g. many PNG tilesets -- and a
        // palettized GPU texture cannot be sampled by the sprite draw path ("Texture
        // doesn't have a palette"); the packer converts every source on the way in. The
        // per-page cap comes from the renderer unless the caller pins one.
        [[nodiscard]] surface_atlases pack_tiles(std::span <const pack_region> regions,
                                                 std::optional <dim> max_page) {
            constexpr auto format = sdlpp::pixel_format_enum::RGBA8888;
            if (!max_page) {
                return pack_regions(regions, format, tile_gutter);
            }
            if (regions.empty()) {
                return surface_atlases{};
            }
            return pack_regions(regions, format, max_page->width, max_page->height, tile_gutter);
        }
    } // namespace

    static tileset_bundle build_bundle_impl(const world_tileset& tileset, std::optional <dim> max_page) {
        // --- CPU phase: decode + gather regions. Nothing is registered yet, so any
        // throw here (a bad image, a packing failure) leaves no resource behind. ---
        std::deque <sdlpp::surface> surfaces; // stable addresses for pack_region::surface
        std::unordered_map <const world_image*, const sdlpp::surface*> decoded;

        auto surface_for = [&] (const world_image* img) -> const sdlpp::surface* {
            const auto it = decoded.find(img);
            if (it != decoded.end()) {
                return it->second;
            }
            surfaces.push_back(decode_world_image(*img));
            const sdlpp::surface* p = &surfaces.back();
            decoded.emplace(img, p);
            return p;
        };

        // Local-id slot count: dense uniform tilesets number 0..tile_count-1, but an
        // image-collection tileset can carry sparse ids past tile_count (e.g. ids up to
        // 13 with tile_count 7). Every id-indexed table below must span the highest id,
        // or those tiles resolve to nothing.
        std::size_t slot_count = tileset.tile_count;
        for (const world_tile& t : tileset.tiles) {
            slot_count = std::max(slot_count, static_cast <std::size_t>(t.id) + 1);
        }

        std::vector <pack_region> regions;
        std::vector <tile_region> metas;
        regions.reserve(slot_count);
        metas.reserve(slot_count);

        for (world_local_tile_id id = 0; id < slot_count; ++id) {
            const world_tile* meta = tileset.tile(id);
            const bool has_own_image = meta != nullptr && meta->image.has_value();
            // A tile resolves if it owns an image or the tileset has a shared image;
            // otherwise it is a gap in a collection tileset -- skip it.
            if (!has_own_image && !tileset.image.has_value()) {
                continue;
            }
            const tile_drawable d = tileset.drawable(id); // non-null image, valid src here
            regions.push_back(pack_region{surface_for(d.image), d.src, nullptr});
            // Bottom-left draw anchor in image-local pixels, folding in the tileset
            // draw offset (d.origin). draw_sprite subtracts the origin, so with the
            // renderer anchoring each cell at its bottom-left this yields Tiled
            // placement: left/bottom-aligned, offset applied with the correct sign,
            // oversized tiles extending upward.
            const point anchor{-d.origin.x, d.src.h - d.origin.y};
            metas.push_back(tile_region{id, anchor});
        }

        const surface_atlases packed = pack_tiles(regions, max_page);

        // --- Registration phase. Each registered handle is recorded in the bundle the
        // instant it is created, and the recording vectors are reserved up front so the
        // recording push_back can never reallocate -- so no handle can be registered
        // yet unrecorded across a throw. If any step then throws, the bundle's
        // destructor unregisters every recorded handle in dependency order, so a failed
        // build leaks nothing. ---
        tileset_bundle bundle;
        bundle.atlases.reserve(packed.pages.size());
        bundle.sheets.reserve(packed.pages.size());
        bundle.animations.reserve(tileset.tile_count);

        // Upload one atlas per page and open a sheet over each.
        std::vector <sprite_sheet> sheets;
        sheets.reserve(packed.pages.size());
        for (const cpu_texture_atlas& page : packed.pages) {
            const gpu_texture_atlas_id atlas = register_atlas(page);
            bundle.atlases.push_back(atlas);
            sheets.emplace_back(atlas);
        }

        // Place every packed tile's visual into its page's sheet. Placements are
        // ordered by (page, input_index), so add_visual order per page gives each
        // tile a stable sheet index we can resolve after registration.
        constexpr auto no_page = static_cast <std::size_t>(-1);
        std::vector <std::size_t> local_page(slot_count, no_page);
        std::vector <std::size_t> local_index(slot_count, 0);
        std::vector <std::size_t> page_next(packed.pages.size(), 0);

        for (const placement& p : packed.placements) {
            const tile_region& tr = metas[p.input_index];
            sheets[p.page].add_visual(sprite_visual{p.bounds, tr.origin});
            local_page[tr.local] = p.page;
            local_index[tr.local] = page_next[p.page]++;
        }

        // Register the sheets, then resolve each tile to its registered visual.
        std::vector <sprite_sheet_id> page_sheet;
        page_sheet.reserve(sheets.size());
        for (sprite_sheet& sheet : sheets) {
            const sprite_sheet_id id = register_sprite_sheet(std::move(sheet));
            bundle.sheets.push_back(id);
            page_sheet.push_back(id);
        }

        bundle.visuals.assign(slot_count, sprite_visual_ref{});
        for (world_local_tile_id id = 0; id < slot_count; ++id) {
            if (local_page[id] != no_page) {
                bundle.visuals[id] = visual_ref(page_sheet[local_page[id]], local_index[id]);
            }
        }

        // Animated tiles: one animation over the frame tiles' visuals and one
        // shared state per animated local id (lockstep playback across instances).
        bundle.states.assign(slot_count, sprite_state_id{});
        for (world_local_tile_id id = 0; id < slot_count; ++id) {
            const std::vector <world_tile_animation_frame>* frames = tileset.animation_of(id);
            if (frames == nullptr || frames->empty()) {
                continue;
            }
            std::vector <sprite_animation_frame> anim_frames;
            anim_frames.reserve(frames->size());
            for (const world_tile_animation_frame& f : *frames) {
                const sprite_visual_ref ref = bundle.visual(f.tile);
                ENFORCE(ref.valid()); // an animation frame must reference a resolvable tile
                anim_frames.push_back(make_sprite_animation_frame(
                    make_sprite_appearance(ref),
                    std::chrono::duration_cast <sprite_animation_duration>(f.duration)));
            }
            const sprite_animation_id anim =
                register_sprite_animation(sprite_animation(std::move(anim_frames)));
            bundle.animations.push_back(anim);
            bundle.states[id] = create_sprite_state(anim);
        }

        return bundle;
    }

    tileset_bundle build_bundle(const world_tileset& tileset) {
        return build_bundle_impl(tileset, std::nullopt);
    }

    tileset_bundle build_bundle(const world_tileset& tileset, dim max_page_size) {
        return build_bundle_impl(tileset, max_page_size);
    }

    void destroy_bundle(tileset_bundle& bundle) noexcept {
        for (const sprite_state_id state : bundle.states) {
            unregister_sprite_state(state);
        }
        for (const sprite_animation_id animation : bundle.animations) {
            unregister_sprite_animation(animation);
        }
        for (const sprite_sheet_id sheet : bundle.sheets) {
            unregister_sprite_sheet(sheet);
        }
        for (const gpu_texture_atlas_id atlas : bundle.atlases) {
            unregister_atlas(atlas);
        }
        // Clear in place now that every id has been unregistered, so a repeated
        // teardown (the destructor, or another call) iterates empty vectors and does
        // nothing.
        bundle.states.clear();
        bundle.animations.clear();
        bundle.sheets.clear();
        bundle.visuals.clear();
        bundle.atlases.clear();
    }

    // RAII: the destructor performs the same dependency-ordered teardown, so every
    // build local, every cache entry, and every partially built bundle releases its
    // registered resources exactly once, on any path.
    tileset_bundle::~tileset_bundle() {
        destroy_bundle(*this);
    }

    // Swap our (empty, freshly default-constructed) vectors with the source's, so the
    // source is left provably empty by exchange semantics -- not by relying on the
    // vector's allocator-dependent moved-from state -- and its destructor no-ops.
    tileset_bundle::tileset_bundle(tileset_bundle&& other) noexcept {
        atlases.swap(other.atlases);
        sheets.swap(other.sheets);
        visuals.swap(other.visuals);
        states.swap(other.states);
        animations.swap(other.animations);
    }

    // Release our own resources first (destroy_bundle leaves our vectors empty), then
    // swap the source in. The source ends up with our now-empty vectors, so overwriting
    // a live bundle can never leak and the source's destructor no-ops.
    tileset_bundle& tileset_bundle::operator=(tileset_bundle&& other) noexcept {
        if (this != &other) {
            destroy_bundle(*this);
            atlases.swap(other.atlases);
            sheets.swap(other.sheets);
            visuals.swap(other.visuals);
            states.swap(other.states);
            animations.swap(other.animations);
        }
        return *this;
    }
}
