//
// Created by igor on 7/18/24.
//

#ifndef  NEUTRINO_SYSTEMS_VIDEO_FONTS_REGISTRY_HH
#define  NEUTRINO_SYSTEMS_VIDEO_FONTS_REGISTRY_HH

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
#include <bsw/mp/type_name/type_name.hpp>

namespace neutrino {
	using rom_font = std::vector <assets::rom_font_glyph>;

	enum bios_rom_font {
		BIOS_8x8,
		BIOS_8x14,
		BIOS_8x16
	};


	using font_t = std::variant <sdl::ttf, assets::bgi_font, rom_font, bios_rom_font, tile_font_spec>;

	class NEUTRINO_EXPORT fonts_registry {
		public:
			[[nodiscard]] font_id_t register_font(sdl::ttf&& font);
			[[nodiscard]] font_id_t register_font(assets::bgi_font&& font);
			[[nodiscard]] font_id_t register_font(const assets::bgi_font& font);
			[[nodiscard]] font_id_t register_font(rom_font&& font);
			[[nodiscard]] font_id_t register_font(const rom_font& font);
			[[nodiscard]] font_id_t register_font(bios_rom_font font);
			[[nodiscard]] font_id_t register_font(tile_font_spec&& font);
			[[nodiscard]] font_id_t register_font(const tile_font_spec& font);

			[[nodiscard]] const font_t& get_font(font_id_t fid) const;
		private:
			std::map <font_id_t, font_t> m_fonts;
	};


}

#endif
