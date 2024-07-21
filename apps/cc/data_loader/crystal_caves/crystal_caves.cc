//
// Created by igor on 7/12/24.
//
#include <array>
#include <fstream>
#include <iostream>
#include "crystal_caves.hh"
#include <bsw/io/binary_reader.hh>

#include "data_loader/data_manager.hh"
#include "sdlpp/video/color_names.hh"

std::vector <raw_map> extract_maps_cc(std::istream& is, const exe_map_props& props) {
	auto unpacked = get_data_manager()->load <neutrino::assets::unpacked_exe>(is);

	bsw::io::memory_input_stream exe_stream(unpacked.data(), static_cast <std::streamoff>(unpacked.size()));
	exe_stream.seekg(props.get_offset(), std::ios::beg);
	bsw::io::binary_reader rdr(exe_stream);

	int level = 0;
	int rows = 0;
	std::vector <raw_map> maps;
	while (true) {
		uint8_t len;
		if (props.get_offset() != 0) {
			rdr >> len;
			if (len != props.get_columns()) {
				break;
			}
		} else {
			len = props.get_columns();
		}

		if (rows == 0) {
			maps.emplace_back(props.get_columns(), props.get_rows(level));
		}

		for (uint8_t i = 0; i < len; i++) {
			uint8_t ch;
			rdr >> ch;
			maps.back().add(ch);
		}

		rows++;
		if (rows >= props.get_rows(level)) {
			level++;
			rows = 0;
		}
	}
	return maps;
}

constexpr int INTRO = 0;
constexpr int FINALE = 1;
constexpr int MAIN = 2;

constexpr int LEVEL(int lvl) {
	return MAIN + lvl;
}

exe_map_props cc1(0x8CE0, 40, 24, {
	                  {INTRO, 5},
	                  {FINALE, 6},
	                  {MAIN, 25},
	                  {LEVEL(7), 23},
	                  {LEVEL(8), 23},
	                  {LEVEL(14), 23}
                  });

exe_map_props cc3(0x8F24, 40, 24, {
	                  {INTRO, 5},
	                  {FINALE, 6},
	                  {MAIN, 25},
	                  {LEVEL(7), 23},
	                  {LEVEL(8), 23},
	                  {LEVEL(14), 23}
                  });

using namespace neutrino;

static std::array <sdl::color, 16> cc_pal = {
	sdl::color(0x00, 0x00, 0x00),
	sdl::color(0xAA, 0x00, 0x00),
	sdl::color(0x00, 0xAA, 0x00),
	sdl::color(0xAA, 0xAA, 0x00),
	sdl::color(0x00, 0x00, 0xAA),
	sdl::color(0xAA, 0x00, 0xAA),
	sdl::color(0x00, 0x55, 0xAA),
	sdl::color(0xAA, 0xAA, 0xAA),
	sdl::color(0x55, 0x55, 0x55),
	sdl::color(0xFF, 0x55, 0x55),
	sdl::color(0x55, 0xFF, 0x55),
	sdl::color(0xFF, 0xFF, 0x55),
	sdl::color(0x55, 0x55, 0xFF),
	sdl::color(0xFF, 0x55, 0xFF),
	sdl::color(0xAA, 0xFF, 0xFF),
	sdl::color(0xFF, 0xFF, 0xFF)
};

assets::tileset load_tileset_cc(std::istream& is) {
	assets::prographx_resource rc(is, true, 0);
	auto ts = get_data_manager()->load <assets::tileset>(rc);
 	sdl::palette palette(cc_pal.begin(), cc_pal.end());
	ts.get_surface().set_palette(palette);
	return ts;
}

assets::tileset load_mini_tileset_cc(std::istream& is) {
	assets::prographx_resource rc(is, true, 0, 50);
	auto ts =  get_data_manager()->load <assets::tileset>(rc);
	sdl::palette palette(cc_pal.begin(), cc_pal.end());
	ts.get_surface().set_palette(palette);
	return ts;
}
