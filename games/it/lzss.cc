//
// Created by igor on 30/09/2026.
//

#include "lzss.hh"
#include <bstream/span_reader.hh>

std::vector<uint8_t> decompress_lzss(std::span<const uint8_t> comp_data, std::size_t decomp_size) {
    std::vector<uint8_t> out;
    if (decomp_size > 0) {
        out.reserve(decomp_size);
    }

    bstream::span_reader reader(comp_data, bstream::endian::little);

    while (!reader.eof()) {
        uint8_t flags = reader.read<uint8_t>();
        for (int bit = 0; bit < 8; ++bit) {
            if (reader.eof()) {
                break;
            }

            if (flags & (1 << bit)) {
                // Literal byte
                out.push_back(reader.read<uint8_t>());
            } else {
                // 2-byte LZSS reference:
                // v10 = (low_byte) | (high_byte << 8)
                // offset (12 bits) = (v10 & 0xFF) | ((v10 & 0xF000) >> 4)
                // length (4 bits)  = ((v10 & 0x0F00) >> 8) + 3
                if (reader.remaining() < 2) {
                    break;
                }
                const uint16_t v10 = reader.read<uint16_t>();

                const std::size_t offset = (v10 & 0xFF) | ((v10 & 0xF000) >> 4);
                const std::size_t length = ((v10 & 0x0F00) >> 8) + 3;

                for (std::size_t j = 0; j < length; ++j) {
                    if (offset == 0 || offset > out.size()) {
                        out.push_back(0);
                    } else {
                        out.push_back(out[out.size() - offset]);
                    }
                }
            }

            if (decomp_size > 0 && out.size() >= decomp_size) {
                out.resize(decomp_size);
                return out;
            }
        }
    }

    if (decomp_size > 0 && out.size() > decomp_size) {
        out.resize(decomp_size);
    }
    return out;
}
