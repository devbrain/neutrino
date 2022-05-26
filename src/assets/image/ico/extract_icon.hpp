#ifndef __SPY_AM_ICONS_EXTRACT_ICON_HPP__
#define __SPY_AM_ICONS_EXTRACT_ICON_HPP__

#include <vector>
#include <string>
#include <iostream>
#include <fstream>

namespace spy
{
    namespace am
    {
		struct icon_disposition_s
		{
			std::size_t offset;
			unsigned w;
			unsigned h;
			int bpp;
		};

		
		 typedef std::vector <icon_disposition_s> dimensions_vec_t;
		 
		 bool extract_icon_dimensions(const char* data, std::size_t size, dimensions_vec_t& out);
		 bool extract_icon_dimensions(const std::string& path, dimensions_vec_t& out);
		 bool extract_icon_dimensions(const std::wstring& path, dimensions_vec_t& out);

		 bool extract_icon (const char* data, std::size_t size, int ordinal, std::vector <unsigned char>& out, unsigned& w, unsigned& h, unsigned ico_dim_limit = 16);
         bool extract_icon (const std::string& path, std::vector <unsigned char>& out, unsigned& w, unsigned& h, unsigned ico_dim_limit = 16);
         bool extract_icon (const std::wstring& path, std::vector <unsigned char>& out, unsigned& w, unsigned& h, unsigned ico_dim_limit = 16);

		 bool extract_icon(const char* data, std::size_t data_size, const icon_disposition_s& d, std::vector <unsigned char>& out, unsigned& w, unsigned& h);
		 bool extract_icon(const std::string& path, const icon_disposition_s& d, std::vector <unsigned char>& out, unsigned& w, unsigned& h);
		 bool extract_icon(const std::wstring& path, const icon_disposition_s& d, std::vector <unsigned char>& out, unsigned& w, unsigned& h);

		 
         bool extract_icon (const std::string& path, const std::string& prefix, unsigned ico_dim_limit = 16);
         bool extract_icon (const std::wstring& path, const std::wstring& prefix, unsigned ico_dim_limit = 16);

         void dump_icons (const std::string& path, const std::string& prefix);
         void dump_icons (const std::wstring& path, const std::wstring& prefix);

         bool extract_from_hicon (void* hicon_ptr, std::vector <unsigned char>& out, unsigned& w, unsigned& h);


		 bool convert_icon(const char* data, std::size_t length, std::vector <unsigned char>& out, unsigned& w, unsigned& h);
		 bool to_png(const std::vector <unsigned char>& in, unsigned w, unsigned h, std::vector <unsigned char>& out);

		 bool load_icon_to_png(const std::string& fname, std::vector<unsigned char>& out, unsigned dim);
    } // ns am
} // ns spy
#endif
