//
// Created by igor on 7/21/24.
//

#include "dialogs.hh"
#include "scenes_registry.hh"
#include <bsw/magic_enum/magic_enum.hpp>
#include "data_loader/crystal_caves/cc1_text_resources.hh"
#include "neutrino/application.hh"

#define dENTRY(S, K, T) std::make_pair(scene_name_t::S, std::make_pair(text_resource::K, T))

static const std::map<scene_name_t, std::pair<text_resource::kind_t, int>> cc1_map = {
    dENTRY(MAIN_DIALOG, MENUS, CC1_TEXT_RESOURCE_MAIN_MENU),
    dENTRY(QUIT_TO_DOS_DIALOG, MENUS, CC1_TEXT_RESOURCE_QUIT),
    dENTRY(INSTRUCTIONS_1, MENUS, CC1_TEXT_RESOURCE_INSTRUCTIONS_1),
    dENTRY(INSTRUCTIONS_2, MENUS, CC1_TEXT_RESOURCE_INSTRUCTIONS_2),
    dENTRY(INSTRUCTIONS_3, MENUS, CC1_TEXT_RESOURCE_INSTRUCTIONS_3),
    dENTRY(INSTRUCTIONS_4, MENUS, CC1_TEXT_RESOURCE_INSTRUCTIONS_4),
    dENTRY(INSTRUCTIONS_5, MENUS, CC1_TEXT_RESOURCE_INSTRUCTIONS_5),
    dENTRY(INSTRUCTIONS_6, MENUS, CC1_TEXT_RESOURCE_INSTRUCTIONS_6),
};

dialogs_factory::dialogs_factory(game_name name, text_resource&& text)
    : name(name),
      text(std::move(text)) {
}

void dialogs_factory::create_dialogs(neutrino::sdl::renderer& renderer) const {
    create_main_dialog_box(renderer);
    create_quit_to_dos_dialog_box(renderer);
    create_instructions(renderer);
}

void dialogs_factory::create_main_dialog_box(neutrino::sdl::renderer& r) const {
    auto scene_name = scene_name_t::MAIN_DIALOG;
    dialog_box::keys_map_t km = {
        {neutrino::sdl::ESCAPE, &dialog_box::pop_scene},
        {neutrino::sdl::Q, []() { dialog_box::push_scene(scene_name_t::QUIT_TO_DOS_DIALOG); }},
        {neutrino::sdl::I, []() { dialog_box::push_scene(scene_name_t::INSTRUCTIONS_1); }}
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

static constexpr std::array<scene_name_t, 6> instruction_names = {
    scene_name_t::INSTRUCTIONS_1,
    scene_name_t::INSTRUCTIONS_2,
    scene_name_t::INSTRUCTIONS_3,
    scene_name_t::INSTRUCTIONS_4,
    scene_name_t::INSTRUCTIONS_5,
    scene_name_t::INSTRUCTIONS_6
};

void dialogs_factory::create_instructions(neutrino::sdl::renderer& r) const {
    for (std::size_t i=0; i<instruction_names.size(); i++) {
        auto scene_name = instruction_names[i];
        if (i < instruction_names.size()-1) {
            dialog_box::keys_map_t km = {
                {neutrino::sdl::ESCAPE, &dialog_box::pop_scene},
                {neutrino::sdl::UNKNOWN, [i]() {
                    dialog_box::replace_scene(instruction_names[i+1]);
                }}
            };
            scenes_registry::instance().add(scene_name, std::make_shared <dialog_box>(r, get_text(scene_name), km));
        } else {
            dialog_box::keys_map_t km = {
                {neutrino::sdl::UNKNOWN, &dialog_box::pop_scene},
            };
            scenes_registry::instance().add(scene_name, std::make_shared <dialog_box>(r, get_text(scene_name), km));
        }
    }
}

std::string dialogs_factory::get_text(scene_name_t scene_name) const {
    const std::map<scene_name_t, std::pair<text_resource::kind_t, int>>* mapping = nullptr;
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

