//
// Created by igor on 7/19/24.
//

#include <neutrino/modules/video/fonts_renderer.hh>
#include <assets/resources/font/rom_font_data_loader.hh>
#include <neutrino/utils/line.hh>
#include <bsw/override.hh>

namespace neutrino {
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

	sdl::area_type get_rendered_dimensions(const font_t& font, const std::string& str) {
		return std::visit([&str](const auto& f) { return get_rendered_dimensions(f, str); }, font);
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

	// ====================================================================================================
	// BGI FONTS
	// ====================================================================================================
	template<typename Callable>
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

	static void render_to_surface(sdl::surface& s, const assets::bgi_font& font, const sdl::color& color,
	                              const std::string& str) {
		int x = 0;
		auto cl = s.map_color(color);
		for (char ch : str) {
			render_glyph(font, x, 0, ch, [cl, &s](int x, int y) {
				uint8_t* row = static_cast <uint8_t*>(s->pixels) + y * s->pitch;
				uint32_t* pixels = reinterpret_cast <uint32_t*>(row) + x;
				*pixels = cl;
			});
			auto i = font.glyphs.find(ch);
			ENFORCE(i != font.glyphs.end());
			x += static_cast <int>(i->second.dimension.w);
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

	// ====================================================================================================
	// ROM FONTS
	// ====================================================================================================

	struct rom_font_holder {
		rom_font_holder(const rom_font& font)
			: m_font(font) {
			ENFORCE(!font.empty());
		}

		[[nodiscard]] const assets::rom_font_glyph& get_glyph(char ch) const {
			return m_font[ch];
		}

		[[nodiscard]] int get_width() const {
			return static_cast <int>(m_font.front().get_width());
		}

		[[nodiscard]] int get_height() const {
			return static_cast <int>(m_font.front().get_height());
		}

		const rom_font& m_font;
	};

	template<typename GlyphHolder>
	static void render_to_surface(sdl::surface& s,
	                              const GlyphHolder& font, const sdl::color& fg_color, const sdl::color& bg_color,
	                              const std::string& str) {
		int x = 0;
		int w = static_cast <int>(font.get_width());
		int h = static_cast <int>(font.get_height());
		auto bg = s.map_color(bg_color);
		auto fg = s.map_color(fg_color);
		for (char ch : str) {
			const auto& glyph = font.get_glyph(ch);
			for (int row = 0; row < h; row++) {
				uint8_t* rowptr = static_cast <uint8_t*>(s->pixels) + row * s->pitch;
				for (int col = 0; col < w; col++) {
					uint32_t* pixels = reinterpret_cast <uint32_t*>(rowptr) + x + col;
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
		sdl::surface s = sdl::surface::make_rgba_32bit(get_rendered_dimensions(font, str));
		render_to_surface(s, rom_font_holder(font), cmd.fg_color, cmd.bg_color, str);
		return s;
	}

	sdl::surface render_font(const rom_font& font, const romfont_render_blended& cmd, const std::string& str) {
		sdl::surface s = sdl::surface::make_rgba_32bit(get_rendered_dimensions(font, str));
		s.set_blend_mode(cmd.mode);
		render_to_surface(s, rom_font_holder(font), cmd.fg_color, cmd.bg_color, str);
		return s;
	}

	sdl::surface render_font(const rom_font& font, const romfont_render_transparent& cmd, const std::string& str) {
		sdl::surface s = sdl::surface::make_rgba_32bit(get_rendered_dimensions(font, str));

		render_to_surface(s, rom_font_holder(font), cmd.color, {0, 0, 0, 0}, str);
		return s;
	}

	sdl::surface render_font(bios_rom_font font, const romfont_render_color& cmd, const std::string& str) {
		sdl::surface s = sdl::surface::make_rgba_32bit(get_rendered_dimensions(font, str));

		switch (font) {
			case BIOS_8x8:
				render_to_surface(s, assets::get_standard_rom_font_8x8(), cmd.fg_color, cmd.bg_color, str);
				break;
			case BIOS_8x14:
				render_to_surface(s, assets::get_standard_rom_font_8x14(), cmd.fg_color, cmd.bg_color, str);
				break;
			case BIOS_8x16:
				render_to_surface(s, assets::get_standard_rom_font_8x16(), cmd.fg_color, cmd.bg_color, str);
				break;
		}
		return s;
	}

	sdl::surface render_font(bios_rom_font font, const romfont_render_blended& cmd, const std::string& str) {
		sdl::surface s = sdl::surface::make_rgba_32bit(get_rendered_dimensions(font, str));
		s.set_blend_mode(cmd.mode);
		switch (font) {
			case BIOS_8x8:
				render_to_surface(s, assets::get_standard_rom_font_8x8(), cmd.fg_color, cmd.bg_color, str);
				break;
			case BIOS_8x14:
				render_to_surface(s, assets::get_standard_rom_font_8x14(), cmd.fg_color, cmd.bg_color, str);
				break;
			case BIOS_8x16:
				render_to_surface(s, assets::get_standard_rom_font_8x16(), cmd.fg_color, cmd.bg_color, str);
				break;
		}
		return s;
	}

	sdl::surface render_font(bios_rom_font font, const romfont_render_transparent& cmd, const std::string& str) {
		sdl::surface s = sdl::surface::make_rgba_32bit(get_rendered_dimensions(font, str));
		constexpr sdl::color transparent(0, 0, 0, 0);
		switch (font) {
			case BIOS_8x8:
				render_to_surface(s, assets::get_standard_rom_font_8x8(), cmd.color, transparent, str);
				break;
			case BIOS_8x14:
				render_to_surface(s, assets::get_standard_rom_font_8x14(), cmd.color, transparent, str);
				break;
			case BIOS_8x16:
				render_to_surface(s, assets::get_standard_rom_font_8x16(), cmd.color, transparent, str);
				break;
		}
		return s;
	}

	// ================================================================================
	// TILED FONT
	// ================================================================================
	sdl::surface render_font(const tile_font_spec& font, const std::string& str) {
		sdl::surface s = sdl::surface::make_rgba_32bit(get_rendered_dimensions(font, str));
		int w = 0;
		for (char c : str) {
			const auto& g = font.get(c);
			ENFORCE(g.is_valid());
			auto [tex_ptr, rect] = g.m_atlas->get(g.m_tile);
			auto tex = const_cast <SDL_Texture*>(tex_ptr->handle());
			SDL_Surface* srf;
			ENFORCE(0 == SDL_LockTextureToSurface(tex, &rect, &srf));
			sdl::rect dst(w, 0, rect.w, rect.h);
			sdl::rect src(0, 0, rect.w, rect.h);
			int rc = SDL_BlitSurface(srf, &src, s.handle(), &dst);
			SDL_UnlockTexture(tex);
			ENFORCE(rc == 0);
			w += rect.w;
		}
		return s;
	}

	sdl::surface render_font(const font_t& font, const std::string& str) {
		return std::visit(
			bsw::overload(
				[&str](const tile_font_spec& ts) {
					return render_font(ts, str);
				},
				[](const auto& f) {
					std::string type(type_name_v <decltype(f)>);
					RAISE_EX("Incorrect font type :", type);
					return sdl::surface{}; // make compiler happy
				}
			),
			font
		);
	}

	sdl::texture render_font(sdl::renderer& r, const tile_font_spec& font, const std::string& str) {
		auto dims = get_rendered_dimensions(font, str);
		sdl::texture tex(r, sdl::pixel_format::RGBA8888, dims, sdl::texture::access::TARGET);
		auto old_target = r.get_target();
		r.set_target(tex);
		int w = 0;
		for (char c : str) {
			const auto& g = font.get(c);
			auto [tex_ptr, rect] = g.m_atlas->get(g.m_tile);
			sdl::rect dst(w, 0, rect.w, rect.h);
			sdl::rect src(0, 0, rect.w, rect.h);
			r.copy(*tex_ptr, src, dst);
			w += rect.w;
		}
		if (old_target) {
			r.set_target(*old_target);
		}
		return tex;
	}

	sdl::texture render_font(sdl::renderer& r, const font_t& font, const std::string& str) {
		return std::visit(
			bsw::overload(
				[&r, &str](const tile_font_spec& ts) {
					return render_font(r, ts, str);
				},
				[](const auto& f) {
					std::string type(type_name_v <decltype(f)>);
					RAISE_EX("Incorrect font type :", type);
					return sdl::texture{}; // make compiler happy
				}
			),
			font
		);
	}
}
