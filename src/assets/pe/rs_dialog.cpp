#include "rs_dialog.hpp"

namespace pefile
{
	static resource_name_c load_resource_name_dlg(bsw::istream_wrapper_c &is, std::size_t maxlen)
	{
		uint16_t descr;
		is >> descr;
		if (descr == 0)
		{
			return resource_name_c();
		}
		else
		{
			if (descr == 0xFFFF)
			{
				uint16_t id;
				is >> id;
				return resource_name_c(id);
			}
			else
			{
				wchar_t ch = static_cast <wchar_t> (descr);
				std::wstring nm;
				nm += ch;
				bool term = false;
				
				for (std::size_t i = 0; i < maxlen; i++)
				{
					is >> ch;
					if (ch)
					{
						nm += ch;
					}
					else
					{
						term = true;
						break;
					}
				}
				if (!term)
				{
					throw std::runtime_error("String should be null terminated");
				}
				return resource_name_c(nm);
			}
		}
	}
	// ---------------------------------------------------------------------------------
	void dialog_c::load(const file_c& file, const resource_c& rn, dialog_c& out)
	{
		const char* file_data = file.file_data();
		const std::size_t file_size = file.file_size();
		auto offs = rn.offset_in_file(file);
		
		if (offs >= file_size)
		{
			return;
		}
		
		bsw::istream_wrapper_c is(file_data + offs, rn.size());

		union
		{
			uint16_t w[2];
			uint32_t dw;
		} u;
		is >> u.dw;
		if (u.w[0] == 1 && u.w[1] == 0xFFFF)
		{
			out.m_extended = true;
			is >> out.m_helpid >> out.m_extstyle >> out.m_style;
		}
		else
		{
			out.m_style = u.dw;
			is >> out.m_extstyle;
		}
		uint16_t num;
		is >> num >> out.m_x >> out.m_y >> out.m_w >> out.m_h;

		out.m_menu = load_resource_name_dlg(is, rn.size());
		out.m_dlg_class = load_resource_name_dlg(is, rn.size());
		out.m_title = load_resource_name_dlg(is, rn.size());

		uint16_t fnt;
		is >> fnt;
		if (fnt != 0x7FFF)
		{
			out.m_font.m_size = fnt;
			if (out.m_extended)
			{
				is >> out.m_font.m_weight;
				uint16_t it;
				is >> it;
				if (it)
				{
					out.m_font.m_italic = true;
				}
			}
			if (fnt)
			{
				is.read_string(out.m_font.m_name, rn.size(), true);
			}
		}
		is.align4();
		for (uint16_t i = 0; i < num; i++)
		{
			dialog_c::control_s ctl;
			if (out.m_extended)
			{
				is >> ctl.m_helpid >> ctl.m_extstyle >> ctl.m_style;
			}
			else
			{
				is >> ctl.m_extstyle >> ctl.m_style;
			}
			is >> ctl.m_x >> ctl.m_y >> ctl.m_w >> ctl.m_h;

			if (out.m_extended)
			{
				uint32_t cid;
				is >> cid;
				ctl.m_id = cid;
			}
			else
			{
				uint16_t cid;
				is >> cid;
				ctl.m_id = cid;
			}
			ctl.m_ctl_class = load_resource_name_dlg(is, rn.size());
			ctl.m_title = load_resource_name_dlg(is, rn.size());
			if (is.size_to_end() == 0 && i + 1 == num)
			{
				is.align4();
				return;
			}
			uint16_t extra_size;
			is >> extra_size;
			if (extra_size)
			{
				if (extra_size < 2)
				{
					throw std::runtime_error("Illegal extasize");
				}
				ctl.m_extra.resize(extra_size - 2);
				is.read(ctl.m_extra.data(), ctl.m_extra.size());
			}
			is.align4();
			out.m_control.push_back(ctl);
		}
	}

}
