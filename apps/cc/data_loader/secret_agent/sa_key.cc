//
// Created by igor on 7/11/24.
//

#include <array>
#include "sa_key.hh"

static constexpr std::array<uint8_t, 28> key{
	0x43, 0x6F, 0x70, 0x79, 0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x31, 0x39, 0x39, 0x31,
	0x20, 0x50, 0x65, 0x64, 0x65, 0x72, 0x20, 0x4A, 0x75, 0x6E, 0x67, 0x63, 0x6B, 0x00
};

static constexpr uint8_t reverse(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

sa_key::sa_key(std::size_t flush_interval)
	: m_flush_interval(flush_interval),
	  m_has_bytes(0),
	  m_current(0) {
}

uint8_t sa_key::operator()(uint8_t in) {
	const uint8_t out = reverse(in) ^ key[m_current];
	m_current++;
	if (m_current >= key.size()) {
		m_current = 0;
	}
	m_has_bytes++;
	if (m_has_bytes>=m_flush_interval) {
		m_has_bytes = 0;
		m_current = 0;
	}
	return out;
}




