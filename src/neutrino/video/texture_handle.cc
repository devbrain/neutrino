//
// Created by igor on 15/11/2026.
//

#include <utility>

#include <failsafe/enforce.hh>

#include "video/texture_table.hh"
#include "video/texture_handle.hh"
#include "video/texture_allocator.hh"
#include "services/service_access.hh"

namespace neutrino::detail {
    // Every call goes through the current allocator and is_valid(): an empty handle, a
    // detached one (the allocator is gone) and a slot the allocator does not hold all get the
    // empty answers. Only get() enforces.

    texture_handle::texture_handle()
        : m_slot_id(texture_table::INVALID_SLOT) {
    }

    texture_handle::~texture_handle() {
        release();
    }

    texture_handle::texture_handle(texture_handle&& other) noexcept
        : m_slot_id(other.m_slot_id) {
        other.m_slot_id = texture_table::INVALID_SLOT;
    }

    texture_handle& texture_handle::operator=(texture_handle&& other) noexcept {
        if (this != &other) {
            release();
            m_slot_id = other.m_slot_id;
            other.m_slot_id = texture_table::INVALID_SLOT;
        }
        return *this;
    }

    const texture& texture_handle::get() const {
        const auto* a = maybe_texture_allocator();
        ENFORCE(a != nullptr && a->is_valid(m_slot_id), "attempt to access non existent texture");
        const auto& slot = a->get(m_slot_id);
        ENFORCE(slot.tex, "attempt to get missing texture at", m_slot_id);
        return *slot.tex;
    }

    texture& texture_handle::get() {
        // The slot's texture is not const; only the lookup above is.
        return const_cast <texture&>(std::as_const(*this).get());
    }

    dim texture_handle::size() const {
        auto* a = maybe_texture_allocator();
        if (!a || !a->is_valid(m_slot_id)) {
            return EMPTY_DIM;
        }
        const auto& slot = a->get(m_slot_id);
        return slot.spec.size;
    }

    bool texture_handle::needs_redraw() const {
        auto* a = maybe_texture_allocator();
        if (!a || !a->is_valid(m_slot_id)) {
            return true;
        }
        const auto& slot = a->get(m_slot_id);
        return slot.needs_redraw;
    }

    void texture_handle::mark_drawn() {
        auto* a = maybe_texture_allocator();
        if (!a || !a->is_valid(m_slot_id)) {
            return;
        }
        auto& slot = a->get(m_slot_id);
        if (slot.tex) {
            slot.needs_redraw = false;
        }
    }

    void texture_handle::clear() {
        auto* a = maybe_texture_allocator();
        if (!a || !a->is_valid(m_slot_id)) {
            return;
        }
        auto& slot = a->get(m_slot_id);
        if (slot.tex) {
            slot.tex->fill(slot.spec.clear_color);
        }
    }

    void texture_handle::invalidate() {
        auto* a = maybe_texture_allocator();
        if (!a || !a->is_valid(m_slot_id)) {
            return;
        }
        auto& slot = a->get(m_slot_id);
        if (slot.tex) {
            slot.tex->fill(slot.spec.clear_color);
        }
        slot.needs_redraw = true;
    }

    void texture_handle::set_clear_color(const color& c) {
        auto* a = maybe_texture_allocator();
        if (!a || !a->is_valid(m_slot_id)) {
            return;
        }
        a->get(m_slot_id).spec.clear_color = c;
    }

    texture_handle::operator bool() const {
        auto* a = maybe_texture_allocator();
        if (!a || !a->is_valid(m_slot_id)) {
            return false;
        }
        return a->get(m_slot_id).tex.has_value();
    }

    bool texture_handle::empty() const {
        const auto* a = maybe_texture_allocator();
        return !a || !a->is_valid(m_slot_id);
    }

    texture_handle::texture_handle(std::size_t slot_id)
        : m_slot_id(slot_id) {
    }

    void texture_handle::release() {
        if (auto* a = maybe_texture_allocator(); a && a->is_valid(m_slot_id)) {
            a->give_back(m_slot_id);
        }
        m_slot_id = texture_table::INVALID_SLOT;
    }
}
