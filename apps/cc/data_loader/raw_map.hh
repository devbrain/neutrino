//
// Created by igor on 7/11/24.
//

#ifndef  RAW_MAP_HH
#define  RAW_MAP_HH

#include <iostream>
#include <vector>
#include <cstdint>
#include <bsw/exception.hh>

class raw_map {
	public:
		using data_t = std::vector <uint8_t>;

		raw_map(unsigned w, unsigned h);

		[[nodiscard]] unsigned get_width() const;
		[[nodiscard]] unsigned get_height() const;

		[[nodiscard]] data_t::const_iterator begin() const;
		[[nodiscard]] data_t::const_iterator end() const;

		[[nodiscard]] data_t::iterator begin();
		[[nodiscard]] data_t::iterator end();

		void add(uint8_t ch);

		uint8_t* data();
		[[nodiscard]] const uint8_t* data() const;

		[[nodiscard]] const uint8_t& get_relative(const data_t::const_iterator& itr, int dx, int dy) const;
		[[nodiscard]] const uint8_t& get_relative(const data_t::iterator& itr, int dx, int dy) const;
		[[nodiscard]] uint8_t& get_relative(const data_t::iterator& itr, int dx, int dy);

	private:
		template <typename Iterator>
		std::size_t get_offset(const Iterator& itr, int dx, int dy) const {
			int curr = static_cast <int>(itr - m_data.begin());
			int d = m_width*dy + dx;
			std::size_t pos = curr + d;
			ENFORCE(pos <m_data.size());
			return pos;
		}
		unsigned m_width;
		unsigned m_height;
		std::vector <uint8_t> m_data;
};

std::ostream& operator <<(std::ostream& os, const raw_map& m);

#endif
