#ifndef __PEFILE_RS_DIALOG_HPP__
#define __PEFILE_RS_DIALOG_HPP__

#include <map>
#include <vector>
#include "resource_directory.hpp"
#include "pefile.hpp"

namespace pefile
{
	class dialog_c
	{
	public:
		static constexpr int resource_id()
		{
			return 5;
		}

		static constexpr bool singleton()
		{
			return false;
		}
	public:
		struct font_s
		{
			font_s()
				: m_size(0),
				m_weight(0),
				m_italic(false)
			{}
			uint16_t m_size;
			uint16_t m_weight;
			bool     m_italic;
			std::wstring m_name;
		};

		struct control_s
		{
			uint32_t m_helpid;
			uint32_t m_extstyle;
			uint32_t m_style;
			uint16_t m_x;
			uint16_t m_y;
			uint16_t m_w;
			uint16_t m_h;
			uint32_t m_id;
			resource_name_c m_ctl_class;
			resource_name_c m_title;
			std::vector <char> m_extra;

		};
	public:
		dialog_c()
			: m_extended(false),
			m_helpid(0),
			m_extstyle(0),
			m_style(0),
			m_x(0),
			m_y(0),
			m_w(0),
			m_h(0)
		{

		}
		static void load(const file_c& file, const resource_c& rn, dialog_c& out);
	public:
		bool     m_extended;
		uint32_t m_helpid;
		uint32_t m_extstyle;
		uint32_t m_style;
		uint16_t m_x;
		uint16_t m_y;
		uint16_t m_w;
		uint16_t m_h;

		resource_name_c m_menu;
		resource_name_c m_dlg_class;
		resource_name_c m_title;

		font_s m_font;

		std::vector <control_s> m_control;

	};


}

#endif
