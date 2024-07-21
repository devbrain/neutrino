//
// Created by igor on 7/18/24.
//

#ifndef TILE_FONT_SPEC_HH
#define TILE_FONT_SPEC_HH

#include <array>
#include <neutrino/systems/video/tile.hh>
#include <neutrino/systems/video/texture_atlas.hh>
#include <neutrino/neutrino_export.hh>


namespace neutrino {
	class NEUTRINO_EXPORT tile_font_spec_builder;

	class NEUTRINO_EXPORT tile_font_spec {
		friend class tile_font_spec_builder;
		public:
			struct glyph {
				glyph();
				glyph(const texture_atlas* atlas, const tile& tile_);
				[[nodiscard]] bool is_valid() const;
				const texture_atlas* m_atlas;
				tile                 m_tile;
			};

			tile_font_spec() = default;
			tile_font_spec(const tile_font_spec&) = default;
			tile_font_spec& operator = (const tile_font_spec&) = default;

			[[nodiscard]] glyph get(char ch) const;
		private:
			explicit tile_font_spec(const std::array <glyph, 256>& fonts);
			std::array <glyph, 256> m_fonts;
	};

	class NEUTRINO_EXPORT tile_font_spec_builder {
		public:
			tile_font_spec_builder();
			explicit tile_font_spec_builder(const texture_atlas* atlas);

			[[nodiscard]] tile_font_spec build() const;

			tile_font_spec_builder& set_atlas(const texture_atlas* atlas);
			tile_font_spec_builder& add(texture_id_t tid, tile_id_t tl, char ch);
			tile_font_spec_builder& add(texture_id_t tid, tile_id_t tl_first, char first, char last);
			tile_font_spec_builder& add(texture_id_t tid, tile_id_t tl_first, const std::string& str);
		private:
			std::array <tile_font_spec::glyph, 256> m_fonts;
			const texture_atlas* m_atlas;
	};
}

#endif
