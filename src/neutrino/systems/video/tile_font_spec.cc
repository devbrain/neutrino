//
// Created by igor on 7/18/24.
//
#include <neutrino/systems/video/tile_font_spec.hh>
#include <bsw/exception.hh>
namespace neutrino {
	tile_font_spec::glyph::glyph()
		: m_atlas(nullptr) {
	}

	tile_font_spec::glyph::glyph(const texture_atlas* atlas, const tile& tile_)
		: m_atlas(atlas),
		  m_tile(tile_) {
	}

	bool tile_font_spec::glyph::is_valid() const {
		if (!m_atlas) {
			return false;
		}
		return m_tile.m_id != EMPTY_TILE_VALUE && m_tile.m_texture != INVALID_TEXTURE_VALUE;
	}

	tile_font_spec::glyph tile_font_spec::get(char ch) const {
		return m_fonts[ch];
	}

	tile_font_spec::tile_font_spec(const std::array <glyph, 256>& fonts)
		: m_fonts(fonts) {
	}

	tile_font_spec_builder::tile_font_spec_builder()
		: m_atlas(nullptr) {
	}

	tile_font_spec_builder::tile_font_spec_builder(const texture_atlas* atlas)
		:m_atlas(atlas) {

	}

	tile_font_spec tile_font_spec_builder::build() const {
		return tile_font_spec(m_fonts);
	}

	tile_font_spec_builder& tile_font_spec_builder::set_atlas(const texture_atlas* atlas) {
		m_atlas = atlas;
		return *this;
	}

	tile_font_spec_builder& tile_font_spec_builder::add(texture_id_t tid, tile_id_t tl, char ch) {
		ENFORCE(m_atlas);
		m_fonts[ch] = tile_font_spec::glyph(m_atlas, tile(tid, tl));
		return *this;
	}

	tile_font_spec_builder& tile_font_spec_builder::add(texture_id_t tid, tile_id_t tl_first, char first, char last) {
		ENFORCE(m_atlas);
		auto first_id = tl_first.value_of();
		for (char ch = first; ch <= last; ch++) {
			m_fonts[ch] = tile_font_spec::glyph(m_atlas, tile(tid, tile_id_t(first_id++)));
		}
		return *this;
	}

	tile_font_spec_builder& tile_font_spec_builder::add(texture_id_t tid, tile_id_t tl_first, const std::string& str) {
		ENFORCE(m_atlas);
		auto first_id = tl_first.value_of();
		for (char ch: str) {
			m_fonts[ch] = tile_font_spec::glyph(m_atlas, tile(tid, tile_id_t(first_id++)));
		}
		return *this;
	}
}
