//
// Created by igor on 7/19/24.
//

#ifndef NEUTRINO_SYSTEMS_VIDEO_FONTS_RENDERER_HH
#define NEUTRINO_SYSTEMS_VIDEO_FONTS_RENDERER_HH

#include <neutrino/systems/video/fonts_registry.hh>
#include <neutrino/neutrino_export.hh>
#include <sdlpp/sdlpp.hh>

namespace neutrino {
	struct NEUTRINO_EXPORT ttf_render_transparent {
		explicit ttf_render_transparent(const sdl::color& color)
			: color(color) {
		}

		sdl::color color;
	};

	struct NEUTRINO_EXPORT ttf_render_blended {
		explicit ttf_render_blended(const sdl::color& color)
			: color(color) {
		}

		sdl::color color;
	};

	struct NEUTRINO_EXPORT ttf_render_lcd {
		explicit ttf_render_lcd(const sdl::color& fg_color)
			: fg_color(fg_color) {
		}

		sdl::color fg_color;
		sdl::color bg_color;
	};

	struct NEUTRINO_EXPORT ttf_render_shaded {
		ttf_render_shaded(const sdl::color& fg_color, const sdl::color& bg_color)
			: fg_color(fg_color),
			  bg_color(bg_color) {
		}

		sdl::color fg_color;
		sdl::color bg_color;
	};

	struct NEUTRINO_EXPORT bgi_render_color {
		explicit bgi_render_color(const sdl::color& color)
			: color(color) {
		}

		sdl::color color;
	};

	struct NEUTRINO_EXPORT bgi_render_blended {
		bgi_render_blended(sdl::blend_mode mode, const sdl::color& color)
			: mode(mode),
			  color(color) {
		}

		sdl::blend_mode mode;
		sdl::color color;
	};

	struct NEUTRINO_EXPORT romfont_render_transparent {
		explicit romfont_render_transparent(const sdl::color& color)
			: color(color) {
		}

		sdl::color color;
	};

	struct NEUTRINO_EXPORT romfont_render_color {
		romfont_render_color(const sdl::color& fg_color, const sdl::color& bg_color)
			: fg_color(fg_color),
			  bg_color(bg_color) {
		}

		sdl::color fg_color;
		sdl::color bg_color;
	};

	struct NEUTRINO_EXPORT romfont_render_blended {
		romfont_render_blended(sdl::blend_mode mode, const sdl::color& fg_color, const sdl::color& bg_color)
			: mode(mode),
			  fg_color(fg_color),
			  bg_color(bg_color) {
		}

		sdl::blend_mode mode;
		sdl::color fg_color;
		sdl::color bg_color;
	};

	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const sdl::ttf& font, const std::string& str);
	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const assets::bgi_font& font, const std::string& str);
	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const rom_font& font, const std::string& str);
	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const bios_rom_font& font, const std::string& str);
	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const tile_font_spec& font, const std::string& str);

	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const font_t& font, const std::string& str);

	NEUTRINO_EXPORT sdl::surface render_font(const sdl::ttf& font,
	                                         const ttf_render_transparent& cmd, const std::string& str);
	NEUTRINO_EXPORT sdl::surface render_font(const sdl::ttf& font,
	                                         const ttf_render_blended& cmd, const std::string& str);
	NEUTRINO_EXPORT sdl::surface render_font(const sdl::ttf& font,
	                                         const ttf_render_shaded& cmd, const std::string& str);
	NEUTRINO_EXPORT sdl::surface render_font(const sdl::ttf& font,
	                                         const ttf_render_lcd& cmd, const std::string& str);

	NEUTRINO_EXPORT sdl::surface render_font(const assets::bgi_font& font,
	                                         const bgi_render_color& cmd, const std::string& str);
	NEUTRINO_EXPORT sdl::surface render_font(const assets::bgi_font& font,
	                                         const bgi_render_blended& cmd, const std::string& str);

	NEUTRINO_EXPORT sdl::surface render_font(const rom_font& font,
	                                         const romfont_render_color& cmd, const std::string& str);
	NEUTRINO_EXPORT sdl::surface render_font(const rom_font& font,
	                                         const romfont_render_blended& cmd, const std::string& str);
	NEUTRINO_EXPORT sdl::surface render_font(const rom_font& font,
	                                         const romfont_render_transparent& cmd, const std::string& str);

	NEUTRINO_EXPORT sdl::surface render_font(bios_rom_font font,
	                                         const romfont_render_color& cmd, const std::string& str);
	NEUTRINO_EXPORT sdl::surface render_font(bios_rom_font font,
	                                         const romfont_render_blended& cmd, const std::string& str);
	NEUTRINO_EXPORT sdl::surface render_font(bios_rom_font font,
	                                         const romfont_render_transparent& cmd, const std::string& str);

	NEUTRINO_EXPORT sdl::surface render_font(const tile_font_spec& font,
	                                         const std::string& str);

	template<typename RenderCommand>
	sdl::surface render_font(const font_t& font, const RenderCommand& cmd, const std::string& str) {
		return std::visit(
			bsw::overload(
				[&str](const tile_font_spec& ts) {
					return render_font(ts, str);
				},
				[&cmd, &str](const auto f) {
					return render_font(f, cmd, str);
				}
			),
			font
		);
	}

	NEUTRINO_EXPORT sdl::surface render_font(const font_t& font, const std::string& str);
	// ---------------------------------------------------------------------------------------------------
	template <typename FontType, typename RenderCommand>
	sdl::texture render_font(sdl::renderer& r, const FontType& font, const RenderCommand& cmd, const std::string& txt) {
		sdl::surface srf = render_font(font, cmd, txt);
		return {r, srf};
	}

	NEUTRINO_EXPORT sdl::texture render_font(sdl::renderer& r, const tile_font_spec& font,
										     const std::string& str);

	template<typename RenderCommand>
	sdl::texture render_font(sdl::renderer& r, const font_t& font, const RenderCommand& cmd, const std::string& str) {
		return std::visit(
			bsw::overload(
				[&r, &str](const tile_font_spec& ts) {
					return render_font(r, ts, str);
				},
				[&r, &cmd, &str](const auto f) {
					return render_font(r, f, cmd, str);
				}
			),
			font
		);
	}

	NEUTRINO_EXPORT sdl::texture render_font(sdl::renderer& r, const font_t& font, const std::string& str);
}

#endif
