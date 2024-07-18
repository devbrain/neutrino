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
	using rom_font = std::vector<assets::rom_font_glyph>;
	enum bios_rom_font {
		BIOS_8x8,
		BIOS_8x14,
		BIOS_8x16
	};
	class NEUTRINO_EXPORT font_renderer {
		public:
			font_id_t register_font(sdl::ttf&& font);
			font_id_t register_font(assets::bgi_font&& font);
			font_id_t register_font(rom_font&& font);
			font_id_t register_font(bios_rom_font font);
			font_id_t register_font(tile_font_spec&& font);

			[[nodiscard]] sdl::area_type get_rendered_dimensions(font_id_t fid, const std::string& str) const;
			sdl::surface render(font_id_t fid, const std::string& str) const;
			void render(sdl::renderer& renderer, font_id_t fid, const std::string& str) const;
		private:
			static sdl::area_type get_rendered_dimensions(const sdl::ttf& font, const std::string& str);
			static sdl::area_type get_rendered_dimensions(const assets::bgi_font& font, const std::string& str);
			static sdl::area_type get_rendered_dimensions(const rom_font& font, const std::string& str);
			static sdl::area_type get_rendered_dimensions(const bios_rom_font& font, const std::string& str);
			static sdl::area_type get_rendered_dimensions(const tile_font_spec& font, const std::string& str);
		private:
			using font_t = std::variant<sdl::ttf, assets::bgi_font, rom_font, bios_rom_font, tile_font_spec>;
			std::map<font_id_t, font_t> m_fonts;
	};
}

#endif
