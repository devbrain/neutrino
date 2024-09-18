//
// Created by igor on 9/17/24.
//

#include "text_resource.hh"
#include <bsw/exception.hh>
#include <bsw/magic_enum/magic_enum.hpp>

std::string text_resource::get(kind_t kind, int id) const {
    auto top = m_text.find(kind);
    if (top == m_text.end()) {
        RAISE_EX("Missing text of kind ", magic_enum::enum_name(kind));
    }
    auto i = top->second.find(id);
    if (i == top->second.end()) {
        RAISE_EX("Missing text of kind ", magic_enum::enum_name(kind), " for id ", id);
    }
    return i->second;
}

void text_resource::bind_text(kind_t kind, int id, const std::string& text) {

    auto [itr, inserted] = m_text.try_emplace(kind, text_map_t{});
    if (itr->second.find(id) != itr->second.end()) {
        RAISE_EX("Menu text ", id, " is already bound to ", magic_enum::enum_name(kind));
    }
    itr->second.emplace(id, text);
}




