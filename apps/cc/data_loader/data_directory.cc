//
// Created by igor on 7/17/24.
//

#include <fstream>
#include <string>
#include <bsw/strings/string_utils.hh>
#include <bsw/exception.hh>
#include "data_directory.hh"
#include "load_picture.hh"
#include "data_manager.hh"
#include "data_loader/crystal_caves/crystal_caves.hh"

static std::map <std::string, data_directory::resource_t> names_mappings = {
	{"CC1.APG", data_directory::CC1_APOGEE_SCREEN},
	{"CC1.TTL", data_directory::CC1_UP_MAIN},
	{"CC1.CDT", data_directory::CC1_DOWN_MAIN},
	{"CC1.EXE", data_directory::CC1_EXE},
	{"CC1.GFX", data_directory::CC1_TILES},
	{"CC1-F1.MNI", data_directory::CC1_MINI_TILES1},
	{"CC1-F2.MNI", data_directory::CC1_MINI_TILES2},
	{"CC1-SPL.MNI", data_directory::CC1_MINI_TILES3},
	{"CC1-1.SND", data_directory::CC1_SOUNDS1},
	{"CC1-2.SND", data_directory::CC1_SOUNDS2},
	{"CC1-3.SND", data_directory::CC1_SOUNDS3},

	{"CC2.APG", data_directory::CC2_APOGEE_SCREEN},
	{"CC2.TTL", data_directory::CC2_UP_MAIN},
	{"CC2.CDT", data_directory::CC2_DOWN_MAIN},
	{"CC2.EXE", data_directory::CC2_EXE},
	{"CC2.GFX", data_directory::CC2_TILES},
	{"CC2-F1.MNI", data_directory::CC2_MINI_TILES1},
	{"CC2-F2.MNI", data_directory::CC2_MINI_TILES2},
	{"CC2-SPL.MNI", data_directory::CC2_MINI_TILES3},
	{"CC2-1.SND", data_directory::CC2_SOUNDS1},
	{"CC2-2.SND", data_directory::CC2_SOUNDS2},
	{"CC2-3.SND", data_directory::CC2_SOUNDS3},

	{"CC3.APG", data_directory::CC3_APOGEE_SCREEN},
	{"CC3.TTL", data_directory::CC3_UP_MAIN},
	{"CC3.CDT", data_directory::CC3_DOWN_MAIN},
	{"CC3.EXE", data_directory::CC3_EXE},
	{"CC3.GFX", data_directory::CC3_TILES},
	{"CC3-F1.MNI", data_directory::CC3_MINI_TILES1},
	{"CC3-F2.MNI", data_directory::CC3_MINI_TILES2},
	{"CC3-SPL.MNI", data_directory::CC3_MINI_TILES3},
	{"CC3-1.SND", data_directory::CC3_SOUNDS1},
	{"CC3-2.SND", data_directory::CC3_SOUNDS2},
	{"CC3-3.SND", data_directory::CC3_SOUNDS3},
};

data_directory::data_directory(const std::filesystem::path& root) {
	for (auto const& dir_entry : std::filesystem::directory_iterator{root}) {
		if (dir_entry.is_regular_file()) {
			auto fname = bsw::to_upper(dir_entry.path().filename().u8string());
			auto i = names_mappings.find(fname);
			if (i != names_mappings.end()) {
				m_fs.insert(std::make_pair(i->second, dir_entry.path()));
			}
		}
	}

	// auto x = get_data_manager()->load<neutrino::assets::unpacked_exe>(*get(CC1_EXE));
	// std::ofstream ofs("cc1.bin", std::ios::binary | std::ios::out);
	// ofs.write(x.data(), x.size());
}

std::unique_ptr <std::istream> data_directory::get(resource_t rc) {
	auto i = m_fs.find(rc);
	ENFORCE(i != m_fs.end());
	auto out = std::make_unique <std::ifstream>(i->second, std::ios::binary | std::ios::in);
	if (!out->good()) {
		RAISE_EX("Can not open file ", i->second);
	}
	return out;
}

neutrino::sdl::surface data_directory::load_picture(resource_t rc) {
	return ::load_picture(*this, rc);
}

static std::tuple <neutrino::sdl::surface, std::vector <neutrino::sdl::rect>> convert_tileset(
	const neutrino::assets::tileset& ts) {
	const auto& s = ts.get_surface();
	auto out_srf = neutrino::sdl::surface::make_rgba_32bit(s.get_dimanesions());
	s.blit(out_srf);
	std::vector <neutrino::sdl::rect> rects;
	constexpr neutrino::sdl::color transparent(0, 0, 0, 0);
	for (const auto& inf : ts) {
		auto src_rect = ts.get_tile(inf.first);
		rects.emplace_back(src_rect);
		if (const auto* bitmap = ts.get_bitmap(inf.first)) {
			for (std::size_t y = 0; y < bitmap->get_height(); y++) {
				uint8_t* row = static_cast <uint8_t*>(out_srf->pixels) + (src_rect.y + y) * out_srf->pitch;
				for (std::size_t x = 0; x < bitmap->get_width(); x++) {
					if (bitmap->data()[bitmap->get_height() * y + x]) {
						uint32_t* px = reinterpret_cast <uint32_t*>(row) + (src_rect.x + x);
						*px = out_srf.map_color(transparent);
					}
				}
			}
		}
	}
	return {std::move(out_srf), rects};
}

std::tuple <neutrino::sdl::surface, std::vector <neutrino::sdl::rect>> data_directory::load_tileset(resource_t rc) {
	bool is_mini = false;
	switch (rc) {
		case CC1_TILES:
		case CC2_TILES:
		case CC3_TILES:
			is_mini = false;
			break;
		case CC1_MINI_TILES1:
		case CC1_MINI_TILES2:
		case CC1_MINI_TILES3:
		case CC2_MINI_TILES1:
		case CC2_MINI_TILES2:
		case CC2_MINI_TILES3:
		case CC3_MINI_TILES1:
		case CC3_MINI_TILES2:
		case CC3_MINI_TILES3:
			is_mini = true;
			break;
		default:
			RAISE_EX("Should not be here");
	}
	if (is_mini) {
		return convert_tileset(load_mini_tileset_cc(*get(rc)));
	}
	return convert_tileset(load_tileset_cc(*get(rc)));
}
