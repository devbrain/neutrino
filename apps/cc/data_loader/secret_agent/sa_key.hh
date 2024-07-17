//
// Created by igor on 7/11/24.
//

#ifndef  SA_KEY_HH
#define  SA_KEY_HH

#include <cstdint>
#include <cstddef>

class sa_key {
	public:
		explicit sa_key(std::size_t flush_interval);
		uint8_t operator () (uint8_t in);
	private:
		std::size_t m_flush_interval;
		std::size_t m_has_bytes;
		std::size_t m_current;
};



#endif
