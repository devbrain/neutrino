//
// Created by igor on 7/18/24.
//


#include <neutrino/systems/video/fonts_registry.hh>
#include <bsw/exception.hh>


namespace neutrino {
	static detail::underlying_font_type s_counter = 0;

	font_id_t fonts_registry::register_font(sdl::ttf&& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{sdl::ttf(std::move(font))}));
		return font_id_t(s_counter);
	}

	font_id_t fonts_registry::register_font(assets::bgi_font&& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{std::move(font)}));
		return font_id_t(s_counter);
	}

	font_id_t fonts_registry::register_font(const assets::bgi_font& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{font}));
		return font_id_t(s_counter);
	}

	font_id_t fonts_registry::register_font(rom_font&& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{std::move(font)}));
		return font_id_t(s_counter);
	}

	font_id_t fonts_registry::register_font(const rom_font& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{font}));
		return font_id_t(s_counter);
	}

	font_id_t fonts_registry::register_font(bios_rom_font font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{font}));
		return font_id_t(s_counter);
	}

	font_id_t fonts_registry::register_font(tile_font_spec&& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{font}));
		return font_id_t(s_counter);
	}

	font_id_t fonts_registry::register_font(const tile_font_spec& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{font}));
		return font_id_t(s_counter);
	}

	const font_t& fonts_registry::get_font(font_id_t fid) const {
		auto i = m_fonts.find(fid);
		ENFORCE(i != m_fonts.end());
		return i->second;
	}

}
