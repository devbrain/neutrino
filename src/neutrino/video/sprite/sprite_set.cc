//
// Created by igor on 12/07/2026.
//

#include <neutrino/video/sprite/sprite_set.hh>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <failsafe/enforce.hh>

#include <neutrino/video/sprites.hh>
#include <neutrino/video/sprite/cpu_texture_atlas.hh>
#include <neutrino/video/sprite/texture_atlas.hh>

#include <sdlpp/video/surface.hh>

#include "services/service_access.hh"
#include "video/sprite/image_decode.hh"
#include "video/sprite/sprites_manager.hh"

namespace neutrino {
    namespace {
        // Decode a sprite_def image to an owned RGBA surface. The already-decoded surface
        // arm is cloned to RGBA8888 (the canonical upload format; the tile path borrows it
        // instead, so that policy stays here); the encoded arms share load_encoded_image.
        sdlpp::surface decode(const world_image& img) {
            if (const auto* surf = std::get_if <image_from_surface>(&img.source)) {
                ENFORCE(surf->pixels != nullptr)("build_sprite_set: image_from_surface has null pixels");
                auto converted = surf->pixels->convert(sdlpp::pixel_format_enum::RGBA8888);
                ENFORCE(converted.has_value())("build_sprite_set: cannot convert source surface");
                return std::move(*converted);
            }
            return details::load_encoded_image(img.source);
        }

        // Grid frames first (named "0".."N-1"), then explicit visuals in declared order.
        // An explicit visual may override a grid name once; a duplicate explicit visual
        // name is a build error. Order-defined, so it matches key_for's declared-order fold.
        std::vector <sprite_visual_def> resolve_visuals(const sprite_def& def, dim image_size) {
            std::vector <sprite_visual_def> resolved;
            std::unordered_map <std::string, std::size_t> index_of;
            std::unordered_set <std::string> grid_names;

            if (def.grid) {
                resolved = expand_grid(*def.grid, image_size);
                for (std::size_t i = 0; i < resolved.size(); ++i) {
                    index_of.emplace(resolved[i].name, i);
                    grid_names.insert(resolved[i].name);
                }
            }

            for (const sprite_visual_def& v : def.visuals) {
                const auto it = index_of.find(v.name);
                if (it == index_of.end()) {
                    index_of.emplace(v.name, resolved.size());
                    resolved.push_back(v);
                    continue;
                }
                // Present already: allowed only when overriding a grid frame (once).
                ENFORCE(grid_names.erase(v.name) > 0)("duplicate sprite visual name: ", v.name);
                resolved[it->second] = v;
            }
            return resolved;
        }

        // The set's single registered sheet, or nullptr if the set is empty or its sheet is
        // no longer registered (e.g. the app is torn down). A sprite_set built by
        // build_sprite_set holds exactly one sheet.
        const sprite_sheet* sheet_of(const render_bundle& bundle) {
            if (bundle.sheets.empty()) {
                return nullptr;
            }
            sprites_manager* manager = maybe_sprites_manager();
            return (manager && manager->contains(bundle.sheets.front()))
                       ? &manager->get(bundle.sheets.front())
                       : nullptr;
        }
    } // namespace

    std::optional <sprite_visual_ref> sprite_set::visual(std::string_view name) const {
        const auto it = visuals_by_name.find(std::string(name));
        if (it == visuals_by_name.end()) {
            return std::nullopt;
        }
        // The name maps survive release() (they are plain members, not registered resources), so a
        // torn-down set would otherwise keep handing out ids pointing at unregistered resources --
        // while the INDEXED accessors, which consult the live sheet, correctly report nothing.
        // Validate against the live resource so both paths agree, and so require_visual on a dead
        // set fails loudly instead of returning a dangling id.
        return find_visual(it->second).has_value() ? std::optional{it->second} : std::nullopt;
    }

    std::size_t sprite_set::visual_count() const {
        const sprite_sheet* sheet = sheet_of(*this);
        return sheet ? sheet->visual_count() : 0;
    }

    std::optional <sprite_visual_ref> sprite_set::visual(std::size_t index) const {
        const sprite_sheet* sheet = sheet_of(*this);
        if (!sheet || index >= sheet->visual_count()) {
            return std::nullopt;
        }
        return sprite_visual_ref{sheets.front(), sheet->visual_id(index)};
    }

    std::optional <rect> sprite_set::frame_rect(std::string_view name) const {
        if (const auto ref = visual(name)) {
            if (const auto v = find_visual(*ref)) {
                return v->texture_rect;
            }
        }
        return std::nullopt;
    }

    std::optional <rect> sprite_set::frame_rect(std::size_t index) const {
        const sprite_sheet* sheet = sheet_of(*this);
        if (!sheet || index >= sheet->visual_count()) {
            return std::nullopt;
        }
        return sheet->visual(sheet->visual_id(index)).texture_rect;
    }

    std::optional <point> sprite_set::origin(std::string_view name) const {
        if (const auto ref = visual(name)) {
            if (const auto v = find_visual(*ref)) {
                return v->origin;
            }
        }
        return std::nullopt;
    }

    std::optional <point> sprite_set::origin(std::size_t index) const {
        const sprite_sheet* sheet = sheet_of(*this);
        if (!sheet || index >= sheet->visual_count()) {
            return std::nullopt;
        }
        return sheet->visual(sheet->visual_id(index)).origin;
    }

    dim sprite_set::bounding_size(std::size_t first) const {
        return bounding_size(first, visual_count());
    }

    dim sprite_set::bounding_size(std::size_t first, std::size_t count) const {
        const sprite_sheet* sheet = sheet_of(*this);
        if (!sheet) {
            return dim{0, 0};
        }
        const std::size_t n = sheet->visual_count();
        if (first >= n) {
            return dim{0, 0};
        }
        const std::size_t last = first + std::min(count, n - first); // clamped to [first, n)
        int w = 0;
        int h = 0;
        for (std::size_t i = first; i < last; ++i) {
            const rect& r = sheet->visual(sheet->visual_id(i)).texture_rect;
            w = std::max(w, r.w);
            h = std::max(h, r.h);
        }
        return dim{w, h};
    }

    std::optional <sprite_metrics> sprite_set::metrics(std::string_view name) const {
        if (const auto ref = visual(name)) {
            if (const auto v = find_visual(*ref)) {
                return v->metrics();
            }
        }
        return std::nullopt;
    }

    std::optional <sprite_metrics> sprite_set::metrics(std::size_t index) const {
        const sprite_sheet* sheet = sheet_of(*this);
        if (!sheet || index >= sheet->visual_count()) {
            return std::nullopt;
        }
        return sheet->visual(sheet->visual_id(index)).metrics();
    }

    std::optional <sprite_animation_id> sprite_set::clip(std::string_view name) const {
        const auto it = clips_by_name.find(std::string(name));
        if (it == clips_by_name.end()) {
            return std::nullopt;
        }
        // Same staleness guard as visual(): clips_by_name outlives release(). A set's animations
        // are registered and torn down together with its sheet, so the sheet's liveness is the
        // set's liveness -- no live sheet means these animation ids no longer address anything.
        return sheet_of(*this) != nullptr ? std::optional{it->second} : std::nullopt;
    }

    // ---- required lookups: a missing entry is a build/config error, not an answer ----------
    // Each reports the SET SIZE alongside the failed key: a wrong sheet (count 0 -- torn down or
    // never built) and an out-of-range index look identical without it.

    sprite_visual_ref sprite_set::require_visual(std::string_view name) const {
        const auto v = visual(name);
        ENFORCE(v.has_value())("sprite_set: no visual named '", name, "' (", visual_count(), " visuals)");
        return *v;
    }

    sprite_visual_ref sprite_set::require_visual(std::size_t index) const {
        const auto v = visual(index);
        ENFORCE(v.has_value())("sprite_set: visual index ", index, " out of range (", visual_count(), ")");
        return *v;
    }

    rect sprite_set::require_frame_rect(std::string_view name) const {
        const auto r = frame_rect(name);
        ENFORCE(r.has_value())("sprite_set: no visual named '", name, "' (", visual_count(), " visuals)");
        return *r;
    }

    rect sprite_set::require_frame_rect(std::size_t index) const {
        const auto r = frame_rect(index);
        ENFORCE(r.has_value())("sprite_set: visual index ", index, " out of range (", visual_count(), ")");
        return *r;
    }

    point sprite_set::require_origin(std::string_view name) const {
        const auto o = origin(name);
        ENFORCE(o.has_value())("sprite_set: no visual named '", name, "' (", visual_count(), " visuals)");
        return *o;
    }

    point sprite_set::require_origin(std::size_t index) const {
        const auto o = origin(index);
        ENFORCE(o.has_value())("sprite_set: visual index ", index, " out of range (", visual_count(), ")");
        return *o;
    }

    sprite_metrics sprite_set::require_metrics(std::string_view name) const {
        const auto m = metrics(name);
        ENFORCE(m.has_value())("sprite_set: no visual named '", name, "' (", visual_count(), " visuals)");
        return *m;
    }

    sprite_metrics sprite_set::require_metrics(std::size_t index) const {
        const auto m = metrics(index);
        ENFORCE(m.has_value())("sprite_set: visual index ", index, " out of range (", visual_count(), ")");
        return *m;
    }

    sprite_animation_id sprite_set::require_clip(std::string_view name) const {
        const auto c = clip(name);
        ENFORCE(c.has_value())("sprite_set: no clip named '", name, "'");
        return *c;
    }

    sprite_set build_sprite_set(const sprite_def& def) {
        // --- CPU phase: decode + resolve. Nothing registered yet, so any throw here
        // (bad image, duplicate name) leaves no resource behind. ---
        sdlpp::surface surface = decode(def.image);
        const dim image_size{surface.get()->w, surface.get()->h};
        const std::vector <sprite_visual_def> resolved = resolve_visuals(def, image_size);

        // The sheet image is already a packed atlas: upload it whole and reference the
        // authored sub-rects in place (repacking would move their coordinates).
        cpu_texture_atlas cpu(std::move(surface),
                              std::vector <cpu_texture_atlas_frame>{
                                  cpu_texture_atlas_frame(rect{0, 0, image_size.width, image_size.height})});

        // --- Registration phase. Each registered id is recorded in `set` the instant it
        // is created, and the owner vectors are reserved up front so the recording
        // push_back can never reallocate (hence never throw) between register and record.
        // So if a later step throws, set's destructor (render_bundle::release) tears down
        // everything already registered, in order, leaking nothing. ---
        sprite_set set;
        set.atlases.reserve(1);
        set.sheets.reserve(1);
        set.animations.reserve(def.clips.size());

        const gpu_texture_atlas_id atlas = register_atlas(cpu, atlas_texture_format::rgba);
        set.atlases.push_back(atlas);

        sprite_sheet sheet(atlas);
        for (const sprite_visual_def& v : resolved) {
            // Carry the trim metadata through to the runtime visual, not just the packed rect and
            // baked pivot: without source_size/trim_offset the authored frame is unrecoverable, and
            // consumers are forced to treat packed pixel sizes as gameplay dimensions.
            const dim source = v.source_size.value_or(dim{v.src.w, v.src.h});
            const point trim = v.trim_offset.value_or(point{0, 0});
            // The trimmed patch must FIT the frame it was trimmed from. The invariant catches
            // incoherent authoring that would otherwise produce silently wrong metrics -- most
            // easily a trim_offset given without a source_size, which leaves the authored size
            // defaulted to the packed size, so trimmed() reads false and logical_bounds_at() no
            // longer contains the visible pixels it is supposed to enclose.
            ENFORCE(trim.x >= 0 && trim.y >= 0
                    && trim.x + v.src.w <= source.width && trim.y + v.src.h <= source.height)
                ("sprite visual '", v.name, "': trimmed rect ", v.src.w, "x", v.src.h,
                 " at offset (", trim.x, ",", trim.y, ") does not fit its source_size ",
                 source.width, "x", source.height,
                 " (a trim_offset requires a matching source_size)");
            sheet.add_visual(v.name, sprite_visual{v.src, baked_visual_origin(v), source, trim});
        }
        const sprite_sheet_id sheet_id = register_sprite_sheet(std::move(sheet));
        set.sheets.push_back(sheet_id);

        set.visuals_by_name.reserve(resolved.size());
        for (const sprite_visual_def& v : resolved) {
            set.visuals_by_name.emplace(v.name, visual_ref(sheet_id, v.name));
        }

        std::unordered_set <std::string> seen_clips;
        set.clips_by_name.reserve(def.clips.size());
        for (const sprite_clip_def& c : def.clips) {
            ENFORCE(seen_clips.insert(c.name).second)("duplicate sprite clip name: ", c.name);

            std::vector <sprite_animation_frame> frames;
            frames.reserve(c.frames.size());
            for (const sprite_frame_def& f : c.frames) {
                const auto it = set.visuals_by_name.find(f.visual);
                ENFORCE(it != set.visuals_by_name.end())
                    ("sprite clip '", c.name, "' references unknown visual: ", f.visual);
                frames.push_back(make_sprite_animation_frame(
                    make_sprite_appearance(it->second, f.flip), f.duration));
            }
            const sprite_animation_id anim =
                register_sprite_animation(sprite_animation(std::move(frames), c.loop));
            set.animations.push_back(anim);
            set.clips_by_name.emplace(c.name, anim);
        }

        return set;
    }
}
