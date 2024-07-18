//
// Created by igor on 7/18/24.
//

#include <algorithm>
#include <neutrino/systems/video/font_renderer.hh>
#include <bsw/exception.hh>

namespace neutrino {
	static detail::underlying_font_type s_counter = 0;

	font_id_t font_renderer::register_font(sdl::ttf&& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{sdl::ttf(std::move(font))}));
		return font_id_t(s_counter);
	}

	font_id_t font_renderer::register_font(assets::bgi_font&& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{std::move(font)}));
		return font_id_t(s_counter);
	}

	font_id_t font_renderer::register_font(rom_font&& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{std::move(font)}));
		return font_id_t(s_counter);
	}

	font_id_t font_renderer::register_font(bios_rom_font font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{font}));
		return font_id_t(s_counter);
	}

	font_id_t font_renderer::register_font(tile_font_spec&& font) {
		m_fonts.insert(std::make_pair(s_counter++, font_t{font}));
		return font_id_t(s_counter);
	}

	sdl::area_type font_renderer::get_rendered_dimensions(font_id_t fid, const std::string& str) const {
		auto i = m_fonts.find(fid);
		ENFORCE(i != m_fonts.end());

		return std::visit([&str](const auto& font) {
			return get_rendered_dimensions(font, str);
		}, i->second);
	}

	sdl::area_type font_renderer::get_rendered_dimensions(const sdl::ttf& font, const std::string& str) {
		auto sz = font.get_text_size(str);
		if (sz) {
			return *sz;
		}
		return {};
	}

	sdl::area_type font_renderer::get_rendered_dimensions(const assets::bgi_font& font, const std::string& str) {
		int w = 0;
		int h = 0;
		for (const auto c : str) {
			auto i = font.glyphs.find(c);
			ENFORCE(i != font.glyphs.end());
			auto d = i->second.dimension;
			w += static_cast <int>(d.w);
			h = std::max(h, static_cast <int>(d.h));
		}
		return {w, h};
	}

	sdl::area_type font_renderer::get_rendered_dimensions(const rom_font& font, const std::string& str) {
		ENFORCE(font.empty());
		auto w = font.front().get_width();
		auto h = font.front().get_height();
		return {static_cast <int>(w * str.size()), static_cast <int>(h)};
	}

	sdl::area_type font_renderer::get_rendered_dimensions(const bios_rom_font& font, const std::string& str) {
		int w = 0;
		int h = 0;
		switch (font) {
			case BIOS_8x8:
				w = 8;
				h = 8;
				break;
			case BIOS_8x14:
				w = 8;
				h = 14;
				break;
			case BIOS_8x16:
				w = 8;
				h = 16;
				break;
			default:
				w = 0;
				h = 0;
				break;
		}
		return {static_cast <int>(w * str.size()), h};
	}

	sdl::area_type font_renderer::get_rendered_dimensions(const tile_font_spec& font, const std::string& str) {
		int w = 0;
		int h = 0;
		for (const auto& c : str) {
			auto glyph = font.get(c);
			ENFORCE(glyph.is_valid());
			auto [_, r] = glyph.m_atlas->get(glyph.m_tile);
			w += r.w;
			h = std::max(h, r.h);
		}
		return {w, h};
	}
}
