//
// Created by igor on 9/17/24.
//

#include "text_resource.hh"
#include <bsw/exception.hh>
#include <bsw/magic_enum/magic_enum.hpp>

std::string text_resource::get(kind_t kind, int id) const {
    if (kind == kind_t::MENUS) {
        auto i = m_menu_text.find(id);
        if (i == m_menu_text.end()) {
            RAISE_EX("Missing menu text for id ", id);
        }
        return i->second;
    } else if (kind == kind_t::IN_GAME) {
        auto i = m_game_text.find(id);
        if (i == m_game_text.end()) {
            RAISE_EX("Missing in game text for id ", id);
        }
        return i->second;
    } else {
        RAISE_EX("Not implemented yet ", magic_enum::enum_name(kind));
    }
}

void text_resource::bind_text(kind_t kind, int id, const std::string& text) {
    if (kind == kind_t::MENUS) {
        if (m_menu_text.find(id) != m_menu_text.end()) {
            RAISE_EX("Menu text ", id, " is already bound");
        }
        m_menu_text[id] = text;
    } else if (kind == kind_t::IN_GAME) {
        if (m_game_text.find(id) != m_game_text.end()) {
            RAISE_EX("Game text ", id, " is already bound");
        }
        m_game_text[id] = text;
    } else {
        RAISE_EX("Not implemented yet ", magic_enum::enum_name(kind));
    }
}




