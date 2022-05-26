#ifndef __SPY_COMMON_AM_ICONS_READER_HPP__
#define __SPY_COMMON_AM_ICONS_READER_HPP__
#include <string>
#include <fstream>
#include <stdint.h>
#include "spy/common/am/icons/icons_container.hpp"

namespace spy
{
	namespace am
	{
		class icons_reader_c
		{
		public:
			explicit icons_reader_c(const std::string& path);
			explicit icons_reader_c(const std::wstring& path);
			icons_reader_c(const char* data, std::size_t size);
			~icons_reader_c();

			icons_reader_c(const icons_reader_c&) = delete;
			icons_reader_c& operator = (const icons_reader_c&) = delete;
			// if w = h = 0, then out is png data
			bool load_icon(std::vector <unsigned char>& out, unsigned& w, unsigned& h, std::streampos offs, uint32_t size);
			bool load_icon_dimensions(unsigned& w, unsigned& h, int& bpp, std::streampos offs, uint32_t size);
			const icons_container_c& container() const;
		private:
			void _load_container();
		private:
			std::ifstream* m_stream;
			std::wstring m_path;
			const char* m_data;
			std::size_t m_size;
			icons_container_c m_container;
		};
	} // ns am
} // ns spy

#endif
