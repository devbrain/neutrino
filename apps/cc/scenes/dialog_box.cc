//
// Created by igor on 7/21/24.
//

#include <algorithm>
#include <utility>
#include "dialog_box.hh"
#include "tile_names.hh"
#include "neutrino/application.hh"
#include "neutrino/modules/video/systems/world_rendering_system.hh"
#include "neutrino/utils/random.hh"

namespace {
    struct star_animation_component {
        star_animation_component()
            : time_in_current_state(0),
              visible(true),
              x(0),
              y(0),
              m_current_frame(0) {
        }

        std::chrono::milliseconds time_in_current_state;
        bool visible;
        int x;
        int y;
        int m_current_frame;
    };
}

class stars_system : public neutrino::ecs::world_rendering_system {
    public:
        explicit stars_system(const neutrino::world_renderer& world_renderer_)
            : world_rendering_system(world_renderer_) {
        }

    private:
        void update(neutrino::ecs::registry& reg,
                    std::chrono::milliseconds delta_t) override {
            auto visual_rect = get_screen_viewport();
            reg.iterate([delta_t, &visual_rect](neutrino::ecs::entity_id_t, star_animation_component& c) {
                const auto& current_frame = ANI_DIALOG_STAR.get_frames()[c.m_current_frame];
                c.time_in_current_state += delta_t;
                if (c.time_in_current_state > current_frame.m_duration) {
                    c.time_in_current_state = std::chrono::milliseconds(0);
                    if (c.visible) {
                        c.m_current_frame++;
                        if (c.m_current_frame >= static_cast <int>(ANI_DIALOG_STAR.get_frames().size())) {
                            c.m_current_frame = 0;
                            c.visible = false;
                        }
                    } else {
                        c.m_current_frame = 0;
                        c.visible = true;
                        const bool vert = neutrino::random::chance(0.5);
                        if (vert) {
                            c.y = visual_rect.y + neutrino::random::get_int(0, visual_rect.h);
                            const bool left = neutrino::random::chance(0.5);;
                            if (left) {
                                c.x = visual_rect.x;
                            } else {
                                c.x = visual_rect.w + visual_rect.x - HUD_TILE_W;
                            }
                        } else {
                            c.x = visual_rect.x + neutrino::random::get_int(0, visual_rect.w);
                            const bool top = neutrino::random::chance(0.5);
                            if (top) {
                                c.y = visual_rect.y;
                            } else {
                                c.y = visual_rect.h + visual_rect.y - HUD_TILE_H;
                            }
                        }
                    }
                }
            });
        }

        void present(neutrino::ecs::registry& reg) override {
            reg.iterate([this](neutrino::ecs::entity_id_t, const star_animation_component& c) {
                if (c.visible) {
                    draw_tile(ANI_DIALOG_STAR, c.m_current_frame, c.x, c.y);
                }
            });
        }
};

static neutrino::sdl::area_type get_dialog_box_dimensions_tiles(const std::string& text) {
    int lines = 0;
    int chars = 0;
    int max_char = 0;
    char last_char = 0;
    for (char c : text) {
        last_char = c;
        if (c == '\n') {
            lines++;
            max_char = std::max(max_char, chars);
            chars = 0;
        } else {
            chars++;
        }
    }
    if (last_char != '\n') {
        lines++;
    }
    max_char = std::max(max_char, chars);
    int tiles_w = max_char + 2;
    int tiles_h = lines + 2;

    return {tiles_w, tiles_h};
}

static void create_dialog_map(neutrino::tiled::world_model& wm,
                              const std::string& text,
                              neutrino::ecs::registry& registry,
                              const neutrino::world_renderer& world_renderer) {
    auto dims = get_dialog_box_dimensions_tiles(text);
    auto tiles_w = dims.w;
    auto tiles_h = dims.h;

    neutrino::tiled::tiles_layer bg(tiles_w, tiles_h, 8, 8);
    for (unsigned int y = 0; y < tiles_h; y++) {
        for (unsigned int x = 0; x < tiles_w; x++) {
            bg.at(x, y) = TILE_DIALOG_BG;
        }
    }
    neutrino::tiled::tiles_layer fg(tiles_w, tiles_h, 8, 8);
    fg.at(0, 0) = TILE_DIALOG_UP_LEFT;
    fg.at(tiles_w - 1, 0) = TILE_DIALOG_UP_RIGHT;
    fg.at(0, tiles_h - 1) = TILE_DIALOG_DOWN_LEFT;
    fg.at(tiles_w - 1, tiles_h - 1) = TILE_DIALOG_DOWN_RIGHT;
    for (unsigned int x = 1; x < tiles_w - 1; x++) {
        fg.at(x, 0) = TILE_DIALOG_UP;
        fg.at(x, tiles_h - 1) = TILE_DIALOG_DOWN;
    }
    for (unsigned int y = 1; y < tiles_h - 1; y++) {
        fg.at(0, y) = TILE_DIALOG_LEFT;
        fg.at(tiles_w - 1, y) = TILE_DIALOG_RIGHT;
    }
    int x = 1;
    int y = 1;
    for (char c : text) {
        if (c == '\n') {
            y++;
            x = 1;
        } else {
            if (c == '{') {
                fg.at(x, y) = ANI_DIALOG_QMARK;
            } else {
                fg.at(x, y) = DIALOG_FONTS.get(c).m_tile;
            }
            x++;
        }
    }
    wm.append(bg);
    wm.append(fg);
    neutrino::tiled::objects_layer stars_layer(registry);
    stars_layer.register_system <stars_system>(world_renderer);
    wm.append(std::move(stars_layer));
    wm.add_animation(ANI_DIALOG_QMARK);
    wm.set_geometry(8, 8, tiles_w, tiles_h);
}

void dialog_box::push_scene(scene_name_t name) {
    scene::push_scene(scenes_registry::instance().get(name));
}

void dialog_box::replace_scene(scene_name_t name) {
    scene::replace_scene(scenes_registry::instance().get(name));
}

dialog_box::dialog_box(neutrino::sdl::renderer& r, const std::string& text, keys_map_t key_mapping)
    : m_world_renderer(r, 8 * get_dialog_box_dimensions_tiles(text)),
      m_key_mapping(std::move(key_mapping)) {
    const auto dims = neutrino::application::instance().get_window_dimensions();
    const auto px_size = m_world_renderer.get_dimension();
    const int x1 = static_cast <int>((dims.w - px_size.w) / 2);
    const int y1 = static_cast <int>((dims.h - px_size.h) / 2);
    m_world_renderer.set_destination_point(neutrino::sdl::point(x1, y1));
    m_stars.bind_component <star_animation_component>(neutrino::ecs::entity_id_t(0));
    create_dialog_map(m_world_model, text, m_stars, m_world_renderer);
}

struct key_event {
    key_event() = default;
    neutrino::sdl::scancode sc;
};

void dialog_box::update(std::chrono::milliseconds delta_time) {
    bool handeled = false;
    if (const auto* ev = get_event <key_event>()) {
        auto i = m_key_mapping.find(ev->sc);
        if (i != m_key_mapping.end()) {
            i->second();
            handeled = true;
        }
    }
    if (!handeled) {
        m_world_renderer.update(delta_time);
    }
}

void dialog_box::render(neutrino::sdl::renderer& renderer) {
    m_world_renderer.present();
}

void dialog_box::initialize() {
    register_event_handler([](const neutrino::sdl::events::keyboard& kb, key_event& ev) {
        if (kb.pressed) {
            ev.sc = kb.scan_code;
        }
        return kb.pressed;
    });

    m_world_renderer.init(get_texture_atlas(), m_world_model);
}

neutrino::scene::flags dialog_box::get_flags() const {
    return flags::TRANSPARENT;
}
