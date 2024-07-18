//
// Created by igor on 7/18/24.
//

#include <algorithm>
#include <neutrino/systems/video/font_renderer.hh>
#include <neutrino/utils/line.hh>
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
			return neutrino::get_rendered_dimensions(font, str);
		}, i->second);
	}

	sdl::area_type get_rendered_dimensions(const sdl::ttf& font, const std::string& str) {
		auto sz = font.get_text_size(str);
		if (sz) {
			return *sz;
		}
		return {};
	}

	sdl::area_type get_rendered_dimensions(const assets::bgi_font& font, const std::string& str) {
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

	sdl::area_type get_rendered_dimensions(const rom_font& font, const std::string& str) {
		ENFORCE(font.empty());
		auto w = font.front().get_width();
		auto h = font.front().get_height();
		return {static_cast <int>(w * str.size()), static_cast <int>(h)};
	}

	sdl::area_type get_rendered_dimensions(const bios_rom_font& font, const std::string& str) {
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

	sdl::area_type get_rendered_dimensions(const tile_font_spec& font, const std::string& str) {
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

	sdl::surface render_font(const sdl::ttf& font, const ttf_render_transparent& cmd, const std::string& str) {
		return font.render_transparent(str, cmd.color);
	}

	sdl::surface render_font(const sdl::ttf& font, const ttf_render_blended& cmd, const std::string& str) {
		return font.render_blended(str, cmd.color);
	}

	sdl::surface render_font(const sdl::ttf& font, const ttf_render_shaded& cmd, const std::string& str) {
		return font.render_shaded(str, cmd.fg_color, cmd.bg_color);
	}

	sdl::surface render_font(const sdl::ttf& font, const ttf_render_lcd& cmd, const std::string& str) {
		return font.render_lcd(str, cmd.fg_color, cmd.bg_color);
	}

	template <typename Callable>
	static void render_glyph(const assets::bgi_font& fnt, int x, int y, char ch, const Callable& plot) {
		const auto& gd = fnt.glyphs.find(ch)->second;

		sdl::point pen(0, 0);
		sdl::point start(x, y);

		for (const auto& c : gd.stroke_commands) {
			if (c.move) {
				pen = c.coords;
			} else {
				utils::line(start + pen, start + c.coords, plot);
				pen = c.coords;
			}
		}
	}

	static void render_to_surface(sdl::surface& s, const assets::bgi_font& font, const sdl::color& color, const std::string& str) {
		int x = 0;
		auto cl = s.map_color(color);
		for (char ch : str) {
			render_glyph(font, x, 0, ch, [cl, &s](int x, int y) {
				uint8_t* row = static_cast <uint8_t*>(s->pixels)+ y * s->pitch;
				uint32_t* pixels = reinterpret_cast<uint32_t*>(row) + x;
				*pixels = cl;
			});
			auto i = font.glyphs.find(ch);
			ENFORCE(i != font.glyphs.end());
			x += static_cast<int>(i->second.dimension.w);
		}
	}

	sdl::surface render_font(const assets::bgi_font& font, const bgi_render_color& cmd, const std::string& str) {
		auto dims = get_rendered_dimensions(font, str);
		sdl::surface s = sdl::surface::make_rgba_32bit(dims.w, dims.h);
		render_to_surface(s, font, cmd.color, str);
		return s;
	}

	sdl::surface render_font(const assets::bgi_font& font, const bgi_render_blended& cmd, const std::string& str) {
		auto dims = get_rendered_dimensions(font, str);
		sdl::surface s = sdl::surface::make_rgba_32bit(dims.w, dims.h);
		s.set_blend_mode(cmd.mode);
		render_to_surface(s, font, cmd.color, str);
		return s;
	}

	static void render_to_surface(sdl::surface& s,
		const rom_font& font, const sdl::color& fg_color, const sdl::color& bg_color, const std::string& str) {
		int x = 0;
		int w = static_cast<int>(font.front().get_width());
		int h = static_cast<int>(font.front().get_height());
		auto bg = s.map_color(bg_color);
		auto fg = s.map_color(fg_color);
		for (char ch : str) {
			const auto& glyph = font[ch];
			for (int row = 0; row < h; row++) {
				uint8_t* rowptr = static_cast <uint8_t*>(s->pixels)+ row * s->pitch;
				for (int col = 0; col<w; col++) {
					uint32_t* pixels = reinterpret_cast<uint32_t*>(rowptr) + x + col;
					if (glyph.is_set(col, row)) {
						*pixels = fg;
					} else {
						*pixels = bg;
					}
				}
			}
			x += w;
		}
	}

	sdl::surface render_font(const rom_font& font, const romfont_render_color& cmd, const std::string& str) {
		ENFORCE(!font.empty());
		auto dims = get_rendered_dimensions(font, str);
		sdl::surface s = sdl::surface::make_rgba_32bit(dims.w, dims.h);
		render_to_surface(s, font, cmd.fg_color, cmd.bg_color, str);
		return s;
	}

	sdl::surface render_font(const rom_font& font, const romfont_render_blended& cmd, const std::string& str) {
		ENFORCE(!font.empty());
		auto dims = get_rendered_dimensions(font, str);
		sdl::surface s = sdl::surface::make_rgba_32bit(dims.w, dims.h);
		s.set_blend_mode(cmd.mode);
		render_to_surface(s, font, cmd.fg_color, cmd.bg_color, str);
		return s;
	}

	sdl::surface render_font(const rom_font& font, const romfont_render_transparent& cmd, const std::string& str) {
		ENFORCE(!font.empty());
		auto dims = get_rendered_dimensions(font, str);
		sdl::surface s = sdl::surface::make_rgba_32bit(dims.w, dims.h);

		render_to_surface(s, font, cmd.color, {0,0,0,0}, str);
		return s;
	}
}
