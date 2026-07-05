//
// Created by igor on 05/07/2026.
//

#include "sprites_manager.hh"

#include <utility>

namespace neutrino {
    sprite_sheet_id sprites_manager::make_id(std::uint32_t value) {
        return sprite_sheet_id(value);
    }

    sprite_sheet_id sprites_manager::create(sprite_sheet sheet) {
        return m_sheets.store(make_id, std::move(sheet));
    }

    sprite_sheet& sprites_manager::get(sprite_sheet_id id) {
        return m_sheets.get(id);
    }

    const sprite_sheet& sprites_manager::get(sprite_sheet_id id) const {
        return m_sheets.get(id);
    }

    void sprites_manager::erase(sprite_sheet_id id) {
        m_sheets.erase(id);
    }
}
