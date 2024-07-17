//
// Created by igor on 7/17/24.
//

#include <fstream>
#include <string>
#include <bsw/strings/string_utils.hh>
#include <bsw/exception.hh>
#include "data_directory.hh"
#include "load_picture.hh"

static std::map <std::string, data_directory::resource_t> names_mappings = {
	{"CC1.APG", data_directory::CC1_APOGEE_SCREEN},
	{"CC1.TTL", data_directory::CC1_UP_MAIN},
	{"CC1.CDT", data_directory::CC1_DOWN_MAIN},
	{"CC1.EXE", data_directory::CC1_EXE},

	{"CC2.APG", data_directory::CC2_APOGEE_SCREEN},
	{"CC2.TTL", data_directory::CC2_UP_MAIN},
	{"CC2.CDT", data_directory::CC2_DOWN_MAIN},
	{"CC2.EXE", data_directory::CC2_EXE},

	{"CC3.APG", data_directory::CC3_APOGEE_SCREEN},
	{"CC3.TTL", data_directory::CC3_UP_MAIN},
	{"CC3.CDT", data_directory::CC3_DOWN_MAIN},
	{"CC3.EXE", data_directory::CC3_EXE},
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
}

std::unique_ptr<std::istream> data_directory::get(resource_t rc) {
	auto i = m_fs.find(rc);
	ENFORCE(i != m_fs.end());
	auto out = std::make_unique<std::ifstream>(i->second, std::ios::binary | std::ios::in);
	if (!out->good()) {
		RAISE_EX("Can not open file ", i->second);
	}
	return out;
}

neutrino::sdl::surface data_directory::load_picture(resource_t rc) {
	return ::load_picture(*this, rc);
}
