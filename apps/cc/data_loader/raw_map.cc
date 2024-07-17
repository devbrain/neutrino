//
// Created by igor on 7/11/24.
//

#include "raw_map.hh"

raw_map::raw_map(unsigned w, unsigned h)
	: m_width(w),
	  m_height(h) {
}

unsigned raw_map::get_width() const {
	return m_width;
}

unsigned raw_map::get_height() const {
	return m_height;
}

raw_map::data_t::const_iterator raw_map::begin() const {
	return m_data.begin();
}

raw_map::data_t::const_iterator raw_map::end() const {
	return m_data.end();
}

raw_map::data_t::iterator raw_map::begin() {
	return m_data.begin();
}

raw_map::data_t::iterator raw_map::end() {
	return m_data.end();
}

void raw_map::add(uint8_t ch) {
	m_data.push_back(ch);
}

uint8_t* raw_map::data() {
	return m_data.data();
}

const uint8_t* raw_map::data() const {
	return m_data.data();
}

const uint8_t& raw_map::get_relative(const data_t::const_iterator& itr, int dx, int dy) const {
	return m_data[get_offset(itr, dx, dy)];
}

const uint8_t& raw_map::get_relative(const data_t::iterator& itr, int dx, int dy) const {
	return m_data[get_offset(itr, dx, dy)];
}

uint8_t& raw_map::get_relative(const data_t::iterator& itr, int dx, int dy) {
	return m_data[get_offset(itr, dx, dy)];
}

std::ostream& operator<<(std::ostream& os, const raw_map& m) {
	unsigned r = 0;
	unsigned c = 0;
	for (const auto ch : m) {
		if (r == 0) {
			std::cout << c << " ::: " ;
			c++;
		}
		os << ch;
		r++;
		if (r >= m.get_width()) {
			os << "\n";
			r = 0;
		}
	}
	return os;
}
