#ifndef __PEFILE_RS_VERSIOB_HPP__
#define __PEFILE_RS_VERSIOB_HPP__

#include <map>
#include <vector>
#include <array>
#include <stdint.h>

#include "resource_directory.hpp"
//#include "pefile.hpp"

namespace pefile
{
	class file_c;
	class version_c
	{
	public:
		typedef std::map <std::string, std::string> kv_map_t;
		typedef std::vector <uint16_t> translations_t;
	public:

		static constexpr int resource_id()
		{
			return 16;
		}

		static constexpr bool singleton()
		{
			return true;
		}

		enum fields_t
		{
			dwSignature,            /* e.g. 0xfeef04bd */
			dwStrucVersion,         /* e.g. 0x00000042 = "0.42" */
			dwFileVersionMS,        /* e.g. 0x00030075 = "3.75" */
			dwFileVersionLS,        /* e.g. 0x00000031 = "0.31" */
			dwProductVersionMS,     /* e.g. 0x00030010 = "3.10" */
			dwProductVersionLS,     /* e.g. 0x00000031 = "0.31" */
			dwFileFlagsMask,        /* = 0x3F for version "0.42" */
			dwFileFlags,            /* e.g. VFF_DEBUG | VFF_PRERELEASE */
			dwFileOS,               /* e.g. VOS_DOS_WINDOWS16 */
			dwFileType,             /* e.g. VFT_DRIVER */
			dwFileSubtype,          /* e.g. VFT2_DRV_KEYBOARD */
			dwFileDateMS,           /* e.g. 0 */
			dwFileDateLS,           /* e.g. 0 */
			MAX_FIELD
		};

		uint32_t operator [] (fields_t f) const;
		std::string operator [] (const std::string& v) const;

		kv_map_t::const_iterator begin() const;
		kv_map_t::const_iterator end() const;
		std::size_t size() const;

		translations_t::const_iterator translations_begin() const;
		translations_t::const_iterator translations_end() const;
		std::size_t translations_size() const;

		static void load(const file_c& file, const resource_c& rn, version_c& out);
	private:
		void _bind(const std::wstring& k, const std::wstring& b);
		void _add_translation(uint16_t x);
	private:
		std::array<uint32_t, MAX_FIELD> m_fields;
		kv_map_t m_kv_map;
		std::vector <uint16_t> m_translations;
	};



} // ns pefile


#endif
