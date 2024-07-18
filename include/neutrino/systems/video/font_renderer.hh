//
// Created by igor on 7/18/24.
//

#ifndef  FONT_RENDERER_HH
#define  FONT_RENDERER_HH

#include <variant>
#include <vector>
#include <map>
#include <string>
#include <sdlpp/sdlpp.hh>
#include <assets/resources/font/bgi_font.hh>
#include <assets/resources/font/rom_font.hh>
#include <neutrino/systems/video/tile_font_spec.hh>
#include <neutrino/systems/video/types.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino {
	using rom_font = std::vector <assets::rom_font_glyph>;

	enum bios_rom_font {
		BIOS_8x8,
		BIOS_8x14,
		BIOS_8x16
	};

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

	using font_render_opts = std::variant <std::monostate,
	                                       ttf_render_transparent, ttf_render_blended,
	                                       ttf_render_shaded, ttf_render_lcd,
	                                       bgi_render_blended, bgi_render_color,
	                                       romfont_render_color, romfont_render_transparent, romfont_render_blended>;

	class NEUTRINO_EXPORT font_renderer {
		public:
			font_id_t register_font(sdl::ttf&& font);
			font_id_t register_font(assets::bgi_font&& font);
			font_id_t register_font(rom_font&& font);
			font_id_t register_font(bios_rom_font font);
			font_id_t register_font(tile_font_spec&& font);

			[[nodiscard]] sdl::area_type get_rendered_dimensions(font_id_t fid, const std::string& str) const;
			sdl::surface render(font_id_t fid, const std::string& str) const;
			void render(sdl::renderer& renderer, const sdl::point& top_left, font_id_t fid,
			            const std::string& str) const;

		private:

		private:
			using font_t = std::variant <sdl::ttf, assets::bgi_font, rom_font, bios_rom_font, tile_font_spec>;
			std::map <font_id_t, font_t> m_fonts;
	};

	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const sdl::ttf& font, const std::string& str);
	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const assets::bgi_font& font, const std::string& str);
	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const rom_font& font, const std::string& str);
	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const bios_rom_font& font, const std::string& str);
	NEUTRINO_EXPORT sdl::area_type get_rendered_dimensions(const tile_font_spec& font, const std::string& str);

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

	NEUTRINO_EXPORT sdl::surface render_font(const bios_rom_font& font,
	                                         const romfont_render_color& cmd, const std::string& str);
	NEUTRINO_EXPORT sdl::surface render_font(const bios_rom_font& font,
	                                         const romfont_render_blended& cmd, const std::string& str);
	NEUTRINO_EXPORT sdl::surface render_font(const bios_rom_font& font,
	                                         const romfont_render_transparent& cmd, const std::string& str);

	NEUTRINO_EXPORT sdl::surface render_font(const tile_font_spec& font,
	                                         const std::string& str);
}

#endif
