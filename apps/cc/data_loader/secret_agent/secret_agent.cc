//
// Created by igor on 7/12/24.
//

#include "secret_agent.hh"
#include "sa_key.hh"
#include "dm.hh"
#include <bsw/io/memory_stream_buf.hh>
#include <bsw/io/binary_reader.hh>

using namespace neutrino;

std::vector<uint8_t> sa_decrypt(const std::filesystem::path& file, std::size_t flush_interval) {
	sa_key key(flush_interval);
	std::ifstream ifs(file, std::ios::binary);
	std::vector<uint8_t> out;
	while (ifs) {
		char ch;
		ifs.read(&ch, 1);
		out.emplace_back(key(ch));
	}
	return out;
}

assets::tileset extract_sa_tileset(const std::filesystem::path& file) {
	auto data = sa_decrypt(file, 8064);
	bsw::io::memory_input_stream is((char*)data.data(), data.size());
	assets::prographx_resource rc(is, true, 8064);
	return dm.load<assets::tileset>(rc);
}

assets::tileset extract_sa_mini_tileset(const std::filesystem::path& file) {
	auto data = sa_decrypt(file, 2048);
	bsw::io::memory_input_stream is((char*)data.data(), data.size());
	assets::prographx_resource rc(is, true, 2048);
	return dm.load<assets::tileset>(rc);
}

std::vector<raw_map> extract_sa_maps(const std::filesystem::path& file) {
	auto data = sa_decrypt(file, 42);
	bsw::io::memory_input_stream is((char*)data.data(), data.size());
	bsw::io::binary_reader rdr(is);

	int r = 0;
	int c = 0;
	std::vector<raw_map> out;
	while (!is.eof()) {
		char ch;
		rdr >> ch;
		if (!is || is.eof()) {
			break;
		}
		if (r == 0 && c == 0) {
			out.emplace_back(42, 48);
		}
		out.back().add(ch);
		c++;
		if (c >= 42) {
			c = 0;
			r++;
		}
		if (r >= 48) {
			r = 0;
		}
	}
	return out;
}