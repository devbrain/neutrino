//
// Created by igor on 12/07/2026.
//

#include <neutrino/video/sprite/atlas_loader.hh>

#include <cstddef>
#include <string>
#include <utility>

#include "utils/json.hh"

namespace neutrino {
    namespace {
        using neutrino::utils::json;

        rect read_rect(const json& j) {
            return rect{j.at("x").get <int>(), j.at("y").get <int>(),
                        j.at("w").get <int>(), j.at("h").get <int>()};
        }
    } // namespace

    sprite_def load_aseprite_atlas(std::string_view text) {
        const json doc = json::parse(text);
        sprite_def def;

        const json& meta = doc.at("meta");
        def.image.source = image_from_disk{meta.value("image", std::string{})};
        if (meta.contains("size")) {
            def.image.width = meta.at("size").value("w", 0u);
            def.image.height = meta.at("size").value("h", 0u);
        }

        // Frames -> explicit named visuals "0".."N-1" (json-array is frame-ordered).
        const json& frames = doc.at("frames");
        def.visuals.reserve(frames.size());
        for (std::size_t i = 0; i < frames.size(); ++i) {
            const json& f = frames[i];
            sprite_visual_def v;
            v.name = std::to_string(i);
            v.src = read_rect(f.at("frame"));
            v.origin = point{0, 0}; // untrimmed top-left; the basic export carries no pivot
            if (f.value("trimmed", false)) {
                const rect sss = read_rect(f.at("spriteSourceSize"));
                v.trim_offset = point{sss.x, sss.y};
                v.source_size = dim{f.at("sourceSize").at("w").get <int>(),
                                    f.at("sourceSize").at("h").get <int>()};
            }
            def.visuals.push_back(std::move(v));
        }

        // Frame tags -> named clips over frame indices.
        if (meta.contains("frameTags")) {
            for (const json& tag : meta.at("frameTags")) {
                sprite_clip_def clip;
                clip.name = tag.value("name", std::string{});
                clip.loop = true;
                const int from = tag.value("from", 0);
                const int to = tag.value("to", 0);
                const std::string dir = tag.value("direction", std::string{"forward"});

                const auto add_frame = [&] (int idx) {
                    if (idx < 0 || static_cast <std::size_t>(idx) >= frames.size()) {
                        return;
                    }
                    const int dur = frames[static_cast <std::size_t>(idx)].value("duration", 100);
                    clip.frames.push_back(sprite_frame_def{
                        std::to_string(idx),
                        sprite_animation_duration{static_cast <float>(dur)},
                        sprite_flip::none});
                };

                if (dir == "reverse") {
                    for (int i = to; i >= from; --i) {
                        add_frame(i);
                    }
                } else { // forward (and any unhandled direction)
                    for (int i = from; i <= to; ++i) {
                        add_frame(i);
                    }
                }
                def.clips.push_back(std::move(clip));
            }
        }

        return def;
    }
}
