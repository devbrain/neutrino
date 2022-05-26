#include <fstream>
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <Windows.h>
#include "spy/common/am/icons/extract_icon.hpp"
#include "spy/common/am/icons/resample.hpp"
#include "spy/common/am/icons/icons_reader.hpp"
#include "spy/common/am/icons/load_icon.hpp"
#include "thirdparty/image/png/lodepng.h"


#include "bsw/fs/mkdir.hpp"
#include "bsw/string.hpp"
#include "bsw/wchar.hpp"

#include "bsw/predef.h"
namespace spy
{
    namespace am
    {
		
		static bool operator < (const icon_disposition_s& a, const icon_disposition_s&b)
		{
			return std::pair<unsigned, unsigned>(a.w, a.h) < std::pair<unsigned, unsigned>(b.w, b.h);
		}

        static int NO_ORDINAL = 0x0001FFFF;


        template <typename STRING>
        static inline STRING path_and_ordinal (const STRING& path, const typename STRING::value_type* sep, STRING& ordinal)
        {
            STRING pth = path;
            bsw::trim (pth);
            typename STRING::size_type p = pth.rfind (sep);
            if (p != STRING::npos)
            {
                ordinal = pth.c_str () + p + 1;
                return STRING (pth.c_str (), pth.c_str () + p);
            }
            return pth;        
        }
		// -----------------------------------------------------------------------------------
		static bool extract_icon(icons_reader_c& rdr, const icon_disposition_s& d, std::vector <unsigned char>& out, unsigned& w, unsigned& h)
		{
			const icons_container_c& container = rdr.container();
			
			if (container.empty())
			{
				return false;
			}
			std::size_t k = 0;
			for (icons_container_c::icon_iterator i = container.icons_begin(); i != container.icons_end(); i++)
			{
				if (k == d.offset)
				{
					return rdr.load_icon(out, w, h, i->second.offset, i->second.size);
				}
				k++;
			}
			return false;
		}
		// ==================================================================================
		static bool extract_best_match(icons_reader_c& rdr, int& ordinal, std::vector <unsigned char>& out, unsigned& w, unsigned& h,
			unsigned ico_dim)
		{

			const icons_container_c& container = rdr.container();

			if (container.empty())
			{
				return false;
			}
			if (ordinal != NO_ORDINAL && ordinal != -1)
			{
				std::streampos offset;
				uint32_t size;
				if (container.get_by_ordinal(static_cast<uint16_t>(ordinal), offset, size))
				{
					return rdr.load_icon(out, w, h, offset, size);
				}
			}

			if (!container.groups_empty())
			{
				icons_container_c::group_iterator i = std::min_element(container.groups_begin(), container.groups_end());
				if (i != container.groups_end())
				{
					std::streampos offset;
					uint32_t size;
					ordinal = i->second;
					if (container.get_by_ordinal(static_cast<uint16_t>(ordinal), offset, size))
					{
						icon_disposition_s dim;
						bool rc = rdr.load_icon_dimensions(dim.w, dim.h, dim.bpp, offset, size);
						if (rc)
						{
							if (dim.bpp > 8 && dim.w <= ico_dim && dim.h <= ico_dim)
							{
								return rdr.load_icon(out, w, h, offset, size);
							}

							auto range = container.group_range(i->first);
							for (auto j = range.first; j != range.second; j++)
							{
								ordinal = j->second;
								if (container.get_by_ordinal(static_cast<uint16_t>(ordinal), offset, size))
								{
									bool rc = rdr.load_icon_dimensions(dim.w, dim.h, dim.bpp, offset, size);
									if (rc)
									{
										if (dim.bpp > 8 && dim.w <= ico_dim && dim.h <= ico_dim)
										{
											return rdr.load_icon(out, w, h, offset, size);
										}
									}
								}
							}
							for (auto j = range.first; j != range.second; j++)
							{
								ordinal = j->second;
								if (container.get_by_ordinal(static_cast<uint16_t>(ordinal), offset, size))
								{
									bool rc = rdr.load_icon_dimensions(dim.w, dim.h, dim.bpp, offset, size);
									if (rc)
									{
										if (dim.bpp > 8)
										{
											return rdr.load_icon(out, w, h, offset, size);
										}
									}
								}
							}
							for (auto j = range.first; j != range.second; j++)
							{
								ordinal = j->second;
								if (container.get_by_ordinal(static_cast<uint16_t>(ordinal), offset, size))
								{
									bool rc = rdr.load_icon_dimensions(dim.w, dim.h, dim.bpp, offset, size);
									if (rc)
									{
										if (dim.bpp <= 8)
										{
											return rdr.load_icon(out, w, h, offset, size);
										}
									}
								}
							}
						}
					}
				}
			}
			if (!container.icons_empty())
			{

				icons_container_c::icon_iterator i = container.icons_begin();
				ordinal = i->first;

				return rdr.load_icon(out, w, h, i->second.offset, i->second.size);
			}
			return false;
		}
		// ==================================================================================
        bool extract_icon (const std::string& path, std::vector <unsigned char>& out, unsigned& w, unsigned& h, unsigned ico_dim_limit)
        {
			try
			{
				std::string sord;
				std::string p = path_and_ordinal <std::string>(path, ",", sord);
				int ordinal = NO_ORDINAL;
				if (!sord.empty())
				{
					std::istringstream is(sord);
					is >> ordinal;
				}
				icons_reader_c rdr(p);
				return extract_best_match(rdr, ordinal, out, w, h, ico_dim_limit);
			}
			catch (std::exception&)
			{
				throw std::runtime_error("failed to extract icon from " + path);
			}
        }
        // ---------------------------------------------------------------------------------------------------
        bool extract_icon (const std::wstring& path, std::vector <unsigned char>& out, unsigned& w, unsigned& h, unsigned ico_dim_limit)
        {
            std::wstring sord;
            std::wstring p = path_and_ordinal <std::wstring> (path, L",", sord);
            int ordinal = NO_ORDINAL;
            if (!sord.empty ())
            {
                std::wistringstream is (sord);
                is >> ordinal;
            }
			icons_reader_c rdr(p);
            return extract_best_match (rdr, ordinal, out, w, h, ico_dim_limit);
        }
        // ---------------------------------------------------------------------------------------------------
		bool extract_icon_dimensions(icons_reader_c& rdr, dimensions_vec_t& out)
		{
			const icons_container_c& container = rdr.container();
			
			if (container.empty())
			{
				return false;
			}
			std::size_t k = 0;
			for (icons_container_c::icon_iterator i = container.icons_begin(); i != container.icons_end(); i++)
			{
				icon_disposition_s dim;
				bool rc = rdr.load_icon_dimensions(dim.w, dim.h, dim.bpp, i->second.offset, i->second.size);
				if (rc)
				{
					dim.offset = k;
					out.push_back(dim);
				}
				k++;
			}
			std::sort(out.begin(), out.end());
			return true;
		}
		// -----------------------------------------------------------------------------------
		bool extract_icon_dimensions(const char* data, std::size_t size, dimensions_vec_t& out)
		{
			icons_reader_c rdr(data, size);
			return extract_icon_dimensions(rdr, out);
		}
		// --------------------------------------------------------------------------------
		bool extract_icon_dimensions(const std::string& path, dimensions_vec_t& out)
		{
			std::string sord;
			std::string p = path_and_ordinal <std::string>(path, ",", sord);
			
			icons_reader_c rdr(p);
			return extract_icon_dimensions (rdr, out);
		}
		// --------------------------------------------------------------------------------
		bool extract_icon_dimensions(const std::wstring& path, dimensions_vec_t& out)
		{
			std::wstring sord;
			std::wstring p = path_and_ordinal <std::wstring>(path, L",", sord);
			
			icons_reader_c rdr(p);
			return extract_icon_dimensions (rdr, out);
		}
		// --------------------------------------------------------------------------------
		bool extract_icon(const char* data, std::size_t data_size, const icon_disposition_s& d, std::vector <unsigned char>& out, unsigned& w, unsigned& h)
		{
			icons_reader_c rdr(data, data_size);
			return extract_icon(rdr, d, out, w, h);
		}
		// --------------------------------------------------------------------------------
		bool extract_icon(const std::string& path, const icon_disposition_s& d, std::vector <unsigned char>& out, unsigned& w, unsigned& h)
		{
			std::string sord;
			std::string p = path_and_ordinal <std::string>(path, ",", sord);
			icons_reader_c rdr(p);
			return extract_icon(rdr, d, out, w, h);
		}
		// --------------------------------------------------------------------------------
		bool extract_icon(const std::wstring& path, const icon_disposition_s& d, std::vector <unsigned char>& out, unsigned& w, unsigned& h)
		{
			std::wstring sord;
			std::wstring p = path_and_ordinal <std::wstring>(path, L",", sord);
			icons_reader_c rdr(p);
			return extract_icon(rdr, d, out, w, h);
		}
		// ---------------------------------------------------------------------------------------------------
		bool extract_icon(const char* data, std::size_t size, int ordinal, std::vector <unsigned char>& out, unsigned& w, unsigned& h, unsigned ico_dim_limit)
		{
			icons_reader_c rdr(data, size);
			return extract_best_match(rdr, ordinal, out, w, h, ico_dim_limit);
		}
        // ---------------------------------------------------------------------------------------------------
        bool extract_icon (const std::string& path, const std::string& prefix, unsigned ico_dim_limit)
        {
            std::string sord;
            std::string p = path_and_ordinal <std::string> (path, ",", sord);
            int ordinal = NO_ORDINAL;
            if (!sord.empty ())
            {
                std::istringstream is (sord);
                is >> sord;
            }
            std::vector <unsigned char> out;
            unsigned w;
            unsigned h;
			icons_reader_c rdr(p);
            if (extract_best_match (rdr, ordinal, out, w, h, ico_dim_limit))
            {
                if (out.empty ())
                {
                    return false;
                }

                bsw::mk_dir (prefix.c_str ());
                std::ostringstream os;
                if (!prefix.empty ())
                {
                    os << prefix << "/";
                }
                os  << ordinal << ".png";
                if (w == 0 && h == 0 && !out.empty ())
                {
                    std::ofstream ofs (os.str (), std::ios::binary);
                    ofs.write ((char*)out.data (), out.size ());
                }
                else
                {
                    lodepng::encode (os.str (), out, w, h);
                }
                return true;
            }
            return false;
        }
        // ---------------------------------------------------------------------------------------------------
        bool extract_icon (const std::wstring& path, const std::wstring& prefix, unsigned ico_dim_limit)
        {
            std::wstring sord;
            std::wstring p = path_and_ordinal <std::wstring> (path, L",", sord);
            int ordinal = NO_ORDINAL;
            if (!sord.empty ())
            {
                std::wistringstream is (sord);
                is >> sord;
            }
            std::vector <unsigned char> out;
            unsigned w;
            unsigned h;
			icons_reader_c rdr(p);
            if (extract_best_match (rdr, ordinal, out, w, h, ico_dim_limit))
            {
                if (out.empty ())
                {
                    return false;
                }

                bsw::mk_dir (bsw::wstring_to_utf8 (prefix).c_str ());
                std::wostringstream os;
                if (!prefix.empty ())
                {
                    os << prefix << L"/";
                }
                os << ordinal << L".png";
                if (w == 0 && h == 0 && !out.empty ())
                {
                    std::ofstream ofs (bsw::wstring_to_utf8 (os.str ()), std::ios::binary);
                    ofs.write ((char*)out.data (), out.size ());
                }
                else
                {
                    lodepng::encode (bsw::wstring_to_utf8 (os.str ()), out, w, h);
                }
                return true;
            }
            return false;
        }
        // ----------------------------------------------------------------------------
		static void dump_icons(icons_reader_c& rdr, const std::string& prefix)
		{
			const icons_container_c& c = rdr.container();
			if (c.empty())
			{
				return;
			}
			bsw::mk_dir(prefix.c_str());
			for (icons_container_c::icon_iterator i = c.icons_begin(); i != c.icons_end(); i++)
			{
				std::vector <unsigned char> out;
				unsigned w, h;
				bool rc = rdr.load_icon(out, w, h, i->second.offset, i->second.size);
				if (rc)
				{

					std::ostringstream str;
					if (!prefix.empty())
					{
						str << prefix << "/" << i->first << ".png";
					}
					else
					{
						str << i->first << ".png";
					}
					std::string opath = str.str();
					if (w == 0 && h == 0 && !out.empty())
					{

						std::ofstream ofs(opath, std::ios::binary);
						ofs.write((char*)out.data(), out.size());
					}
					else
					{

						lodepng::encode(opath, out, w, h);
					}
				}
			}
		}
        
		void dump_icons(const std::string& path, const std::string& prefix)
		{
			icons_reader_c rdr(path);
			dump_icons(rdr, prefix);
		}
        // ----------------------------------------------------------------------------
        void dump_icons (const std::wstring& path, const std::wstring& prefix)
        {
			icons_reader_c rdr(path);
			dump_icons(rdr, bsw::wstring_to_utf8(prefix));
        }
        // ------------------------------------------------------------------------------------
#if defined(PREDEF_PLATFORM_WIN32)
        bool extract_from_hicon (void* hicon_ptr, std::vector <unsigned char>& out, unsigned& w, unsigned& h)
        {
            if (hicon_ptr == NULL)
            {
                return false;
            }
            HICON* hicon = (HICON*) hicon_ptr;
            if (*hicon == NULL)
            {
                return false;
            }
            return load_icon (*hicon, out, w, h);
        }
#endif
		// ----------------------------------------------------------------------------------------------------------------

		bool convert_icon(const char* data, std::size_t length, std::vector <unsigned char>& out, unsigned& w, unsigned& h)
		{
			icons_reader_c rdr(data, length);
			
			const icons_container_c& container = rdr.container();
			if (!container.icons_empty())
			{
				icons_container_c::icon_iterator i = container.icons_begin();
				return rdr.load_icon(out, w, h, i->second.offset, i->second.size);
			}
			return false;
		}
		// ----------------------------------------------------------------------------------------------------------------
		bool to_png(const std::vector <unsigned char>& in, unsigned w, unsigned h, std::vector <unsigned char>& out)
		{
			lodepng::encode(out, in, w, h);
			return true;

		}
		// ----------------------------------------------------------------------------------------------
		bool load_icon_to_png(const std::string& fname, std::vector<unsigned char>& out, unsigned dim)
		{
			std::vector <unsigned char> buff;
			unsigned w, h;
			spy::am::extract_icon(fname, buff, w, h, 32);
			if (!buff.empty())
			{
				if (w > 0 && h > 0)
				{
					lodepng::encode(out, buff, w, h);
				}
				else
				{
					std::vector <unsigned char> image;
					const unsigned rc = lodepng::decode(image, w, h, buff, LCT_RGBA, 8);
					out = buff;
				}

				if (w > dim || h > dim)
				{
					reduce_png(out, dim);
				}
				return true;
			}
			else
			{
				return false;
			}
		}
    } // ns am
} // ns spy
