//
// Created by igor on 7/21/24.
//

#include "dialogs.hh"
#include "scenes_registry.hh"
#include <bsw/magic_enum/magic_enum.hpp>
#include "data_loader/crystal_caves/cc1_text_resources.hh"
#include "neutrino/application.hh"

#define dENTRY(S, K, T) std::make_pair(scene_name_t::S, std::make_pair(text_resource::K, T))

static const std::map <scene_name_t, std::pair <text_resource::kind_t, int>> cc1_map = {
    dENTRY(MAIN_DIALOG, MENUS, CC1_TEXT_RESOURCE_MAIN_MENU),
    dENTRY(QUIT_TO_DOS_DIALOG, MENUS, CC1_TEXT_RESOURCE_QUIT),
    dENTRY(INSTRUCTIONS_1, INSTRUCTIONS, CC1_TEXT_RESOURCE_INSTRUCTIONS_1),
    dENTRY(INSTRUCTIONS_2, INSTRUCTIONS, CC1_TEXT_RESOURCE_INSTRUCTIONS_2),
    dENTRY(INSTRUCTIONS_3, INSTRUCTIONS, CC1_TEXT_RESOURCE_INSTRUCTIONS_3),
    dENTRY(INSTRUCTIONS_4, INSTRUCTIONS, CC1_TEXT_RESOURCE_INSTRUCTIONS_4),
    dENTRY(INSTRUCTIONS_5, INSTRUCTIONS, CC1_TEXT_RESOURCE_INSTRUCTIONS_5),
    dENTRY(INSTRUCTIONS_6, INSTRUCTIONS, CC1_TEXT_RESOURCE_INSTRUCTIONS_6),
    dENTRY(INSTRUCTIONS_7, INSTRUCTIONS, CC1_TEXT_RESOURCE_INSTRUCTIONS_7),
    dENTRY(STORY1, STORY, CC1_TEXT_RESOURCE_STORY1),
    dENTRY(STORY2, STORY, CC1_TEXT_RESOURCE_STORY2),
    dENTRY(ORDER1, ORDERING, CC1_TEXT_RESOURCE_ORDER1),
    dENTRY(ORDER2, ORDERING, CC1_TEXT_RESOURCE_ORDER2),
    dENTRY(ORDER3, ORDERING, CC1_TEXT_RESOURCE_ORDER3),
    dENTRY(FORDER1, ORDERING, CC1_TEXT_RESOURCE_FORDER1),
    dENTRY(FORDER2, ORDERING, CC1_TEXT_RESOURCE_FORDER2),
    dENTRY(FORDER3, ORDERING, CC1_TEXT_RESOURCE_FORDER3),
    dENTRY(FORDER4, ORDERING, CC1_TEXT_RESOURCE_FORDER4),
    dENTRY(BBS, ORDERING, CC1_TEXT_RESOURCE_BBS),
    dENTRY(ABOUT_APOGEE, MENUS, CC1_TEXT_RESOURCE_ABOUT_APOGEE),
    dENTRY(ABOUT_VIDEO, MENUS, CC1_TEXT_RESOURCE_VIDEO),

};

static constexpr std::array <scene_name_t, 7> instruction_names = {
    scene_name_t::INSTRUCTIONS_1,
    scene_name_t::INSTRUCTIONS_2,
    scene_name_t::INSTRUCTIONS_3,
    scene_name_t::INSTRUCTIONS_4,
    scene_name_t::INSTRUCTIONS_5,
    scene_name_t::INSTRUCTIONS_6,
    scene_name_t::INSTRUCTIONS_7,
};

static constexpr std::array <scene_name_t, 2> story_names = {
    scene_name_t::STORY1,
    scene_name_t::STORY2
};

static constexpr std::array <scene_name_t, 3> order_names = {
    scene_name_t::ORDER1,
    scene_name_t::ORDER2,
    scene_name_t::ORDER3
};

static constexpr std::array <scene_name_t, 4> forder_names = {
    scene_name_t::FORDER1,
    scene_name_t::FORDER2,
    scene_name_t::FORDER3,
    scene_name_t::FORDER4
};

static constexpr std::array <scene_name_t, 1> bbs_names = {
    scene_name_t::BBS,
};

static constexpr std::array <scene_name_t, 1> about_apogee_names = {
    scene_name_t::ABOUT_APOGEE,
};

static constexpr std::array <scene_name_t, 1> about_video_names = {
    scene_name_t::ABOUT_VIDEO,
};

template<std::size_t N, typename TextGetter>
void create_many(neutrino::sdl::renderer& r, const std::array <scene_name_t, N>& array, const TextGetter& tg) {
    for (std::size_t i = 0; i < array.size(); i++) {
        auto scene_name = array[i];
        if (i < array.size() - 1) {
            dialog_box::keys_map_t km = {
                {neutrino::sdl::ESCAPE, &dialog_box::pop_scene},
                {
                    neutrino::sdl::UNKNOWN, [i, &array]() {
                        dialog_box::replace_scene(array[i + 1]);
                    }
                }
            };
            scenes_registry::instance().add(scene_name, std::make_shared <dialog_box>(r, tg(scene_name), km));
        } else {
            dialog_box::keys_map_t km = {
                {neutrino::sdl::UNKNOWN, &dialog_box::pop_scene},
            };
            scenes_registry::instance().add(scene_name, std::make_shared <dialog_box>(r, tg(scene_name), km));
        }
    }
}

dialogs_factory::dialogs_factory(game_name name, text_resource&& text)
    : name(name),
      text(std::move(text)) {
}

void dialogs_factory::create_dialogs(neutrino::sdl::renderer& renderer) const {
    create_main_dialog_box(renderer);
    create_quit_to_dos_dialog_box(renderer);
    create_many(renderer, instruction_names, [this](scene_name_t scene_name) { return get_text(scene_name); });
    create_many(renderer, story_names, [this](scene_name_t scene_name) { return get_text(scene_name); });
    create_many(renderer, order_names, [this](scene_name_t scene_name) { return get_text(scene_name); });
    create_many(renderer, forder_names, [this](scene_name_t scene_name) { return get_text(scene_name); });
    create_many(renderer, bbs_names, [this](scene_name_t scene_name) { return get_text(scene_name); });
    create_many(renderer, about_apogee_names, [this](scene_name_t scene_name) { return get_text(scene_name); });
    create_many(renderer, about_video_names, [this](scene_name_t scene_name) { return get_text(scene_name); });
}

void dialogs_factory::create_main_dialog_box(neutrino::sdl::renderer& r) const {
    auto scene_name = scene_name_t::MAIN_DIALOG;
    dialog_box::keys_map_t km = {
        {neutrino::sdl::ESCAPE, &dialog_box::pop_scene},
        {neutrino::sdl::Q, []() { dialog_box::push_scene(scene_name_t::QUIT_TO_DOS_DIALOG); }},
        {neutrino::sdl::I, []() { dialog_box::replace_scene(scene_name_t::INSTRUCTIONS_1); }},
        {neutrino::sdl::S, []() { dialog_box::replace_scene(scene_name_t::STORY1); }},
        {neutrino::sdl::O, []() { dialog_box::replace_scene(scene_name_t::ORDER1); }},
        {neutrino::sdl::F, []() { dialog_box::replace_scene(scene_name_t::FORDER1); }},
        {neutrino::sdl::C, []() { dialog_box::replace_scene(scene_name_t::BBS); }},
        {neutrino::sdl::A, []() { dialog_box::replace_scene(scene_name_t::ABOUT_APOGEE); }},
          {neutrino::sdl::V, []() { dialog_box::replace_scene(scene_name_t::ABOUT_VIDEO); }},
    };
    scenes_registry::instance().add(scene_name, std::make_shared <dialog_box>(r, get_text(scene_name), km));
}

void dialogs_factory::create_quit_to_dos_dialog_box(neutrino::sdl::renderer& r) const {
    auto scene_name = scene_name_t::QUIT_TO_DOS_DIALOG;
    dialog_box::keys_map_t km = {
        {neutrino::sdl::ESCAPE, &dialog_box::pop_scene},
        {neutrino::sdl::Y, []() { neutrino::application::instance().quit(); }},
        {neutrino::sdl::N, &dialog_box::pop_scene}
    };
    scenes_registry::instance().add(scene_name, std::make_shared <dialog_box>(r, get_text(scene_name), km));
}

std::string dialogs_factory::get_text(scene_name_t scene_name) const {
    const std::map <scene_name_t, std::pair <text_resource::kind_t, int>>* mapping = nullptr;
    switch (name) {
        case game_name::CC1:
            mapping = &cc1_map;
            break;
        default:
            RAISE_EX("Not implemented yet");
    }

    auto itr = mapping->find(scene_name);
    if (itr == mapping->end()) {
        RAISE_EX("Can not find mapping for ", magic_enum::enum_name(name), "/", magic_enum::enum_name(scene_name));
    }
    auto [kind, id] = itr->second;
    return text.get(kind, id);
}
