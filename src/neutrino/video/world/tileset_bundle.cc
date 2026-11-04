//
// Created by igor on 07/07/2026.
//

#include <neutrino/video/world/tileset_bundle.hh>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <failsafe/enforce.hh>

#include <neutrino/video/sprites.hh>
#include <neutrino/video/atlas/image_atlas.hh>
#include <neutrino/video/image/image_bank.hh>
#include <neutrino/video/color.hh>

#include "video/sprite/image_decode.hh"

namespace neutrino {
    namespace {
        // One packed tile: the local id and draw origin to carry onto its visual.
        struct tile_region {
            world_local_tile_id local;
            point               origin;
        };

        // Tiles are packed in a canonical RGBA format. Source images may be palettized
        // (indexed) -- e.g. many PNG tilesets -- and a palettized GPU texture cannot be
        // sampled by the sprite draw path ("Texture doesn't have a palette").
        constexpr auto tile_format = pixel_format::RGBA8888;

        // Copy @p src of @p rgba (already in tile_format, blending off) into its own picture.
        [[nodiscard]] image crop_tile(const image& rgba, const rect& src) {
            // NB: ENFORCE the has_value(), not the expected itself -- ENFORCE moves the value out.
            auto tile = image::create_rgb(src.w, src.h, tile_format);
            ENFORCE(tile.has_value(), "Can not create a tile picture", src.w, "x", src.h);
            ENFORCE(tile->fill(color{0, 0, 0, 0}).has_value());
            ENFORCE(rgba.blit_to <rect>(*tile, src, point{0, 0}).has_value());
            return std::move(*tile);
        }
    } // namespace

    static tileset_bundle build_bundle_impl(const world_tileset& tileset, std::optional <dim> max_page) {
        // --- CPU phase: decode, crop every tile into an image bank and pack it. Nothing
        // is registered yet, so any throw here (a bad image, a packing failure) leaves no
        // resource behind. ---
        std::unordered_map <const world_image*, image> decoded; // each source, in tile_format

        auto rgba_for = [&] (const world_image* img) -> const image& {
            const auto it = decoded.find(img);
            if (it != decoded.end()) {
                return it->second;
            }
            std::optional <image> owned;
            const image* source = nullptr;
            if (const auto* s = std::get_if <image_from_surface>(&img->source)) {
                // Already decoded: read the producer's surface. The world_image (and its
                // shared_ptr) outlives this synchronous decode->pack.
                ENFORCE(s->pixels != nullptr, "image_from_surface has null pixels");
                source = s->pixels.get();
            } else {
                owned = details::load_encoded_image(img->source);
                source = &*owned;
            }
            // Convert once per source (a colour key becomes alpha 0), with blending off so
            // the crops copy partial alpha as-is.
            auto rgba = source->convert(tile_format);
            ENFORCE(rgba.has_value(), "Can not convert a tileset image to RGBA");
            ENFORCE(rgba->set_blend_mode(sdlpp::blend_mode::none).has_value());
            return decoded.emplace(img, std::move(*rgba)).first->second;
        };

        // Local-id slot count: dense uniform tilesets number 0..tile_count-1, but an
        // image-collection tileset can carry sparse ids past tile_count (e.g. ids up to
        // 13 with tile_count 7). Every id-indexed table below must span the highest id,
        // or those tiles resolve to nothing. effective_tile_count derives the grid size
        // from the shared image when tile_count was omitted, so a direct-built or
        // hand-authored uniform tileset does not build zero visuals.
        std::size_t slot_count = tileset.effective_tile_count();
        for (const world_tile& t : tileset.tiles) {
            slot_count = std::max(slot_count, static_cast <std::size_t>(t.id) + 1);
        }

        image_bank bank;
        std::vector <tile_region> metas;
        bank.entries.reserve(slot_count);
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
            bank.entries.push_back(image_bank::entry{crop_tile(rgba_for(d.image), d.src),
                                                     static_cast <std::uint32_t>(id)});
            // Bottom-left draw anchor in image-local pixels, folding in the tileset
            // draw offset (d.origin). draw_sprite subtracts the origin, so with the
            // renderer anchoring each cell at its bottom-left this yields Tiled
            // placement: left/bottom-aligned, offset applied with the correct sign,
            // oversized tiles extending upward.
            const point anchor{-d.origin.x, d.src.h - d.origin.y};
            metas.push_back(tile_region{id, anchor});
        }

        tileset_bundle bundle;
        bundle.visuals.assign(slot_count, sprite_visual_ref{});
        bundle.states.assign(slot_count, sprite_state_id{});
        if (bank.entries.empty()) {
            return bundle; // nothing to upload
        }

        image_atlas cpu = build_atlas(bank, tile_format, max_page);

        // One visual per picture; picture i is tile metas[i].
        sprite_sheet sheet(cpu.id());
        for (std::size_t i = 0; i < metas.size(); ++i) {
            const dim size = cpu.picture_size(i);
            sheet.add_visual(sprite_visual{rect{0, 0, size.width, size.height}, metas[i].origin, {}, {}, i});
        }

        // --- Registration phase. Each registered handle is recorded in the bundle the
        // instant it is created, and the recording vectors are reserved up front so the
        // recording push_back can never reallocate -- so no handle can be registered
        // yet unrecorded across a throw. If any step then throws, the bundle's
        // destructor unregisters every recorded handle in dependency order, so a failed
        // build leaks nothing. ---
        bundle.atlases.reserve(1);
        bundle.sheets.reserve(1);
        bundle.animations.reserve(tileset.tile_count);

        bundle.atlases.push_back(register_atlas(std::move(cpu)));
        const sprite_sheet_id sheet_id = register_sprite_sheet(std::move(sheet));
        bundle.sheets.push_back(sheet_id);

        for (std::size_t i = 0; i < metas.size(); ++i) {
            bundle.visuals[metas[i].local] = visual_ref(sheet_id, i);
        }

        // Animated tiles: one animation over the frame tiles' visuals and one
        // shared state per animated local id (lockstep playback across instances).
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
        // The render_bundle base owns and unregisters atlas/sheet/animation/state in
        // dependency order; visuals are lightweight refs (nothing to unregister).
        bundle.release();
        bundle.visuals.clear();
    }

    // Delegate the owned ids to the base's swap-move, then swap visuals so the moved-from
    // bundle is provably empty (not std::vector's valid-but-unspecified state). The
    // destructor stays implicit: the base releases.
    tileset_bundle::tileset_bundle(tileset_bundle&& other) noexcept
        : render_bundle(std::move(other)) {
        visuals.swap(other.visuals);
    }

    tileset_bundle& tileset_bundle::operator=(tileset_bundle&& other) noexcept {
        if (this != &other) {
            render_bundle::operator=(std::move(other)); // release ours, swap the base in
            // The base clears its own vectors, but not our facade refs; drop them before
            // the swap so the source is left with an empty (not our stale) visuals.
            visuals.clear();
            visuals.swap(other.visuals);
        }
        return *this;
    }
}
