#include "spy/common/am/icons/icons_reader.hpp"
#include "spy/common/am/icons/pe_image.hpp"
#include "spy/common/am/icons/ne_image.hpp"
#include "spy/common/am/icons/ico_image.hpp"
#include "spy/common/am/icons/load_icon.hpp"
#include "bsw/fs/istream_wrapper.hpp"
#include "bsw/wchar.hpp"

namespace spy
{
	namespace am
	{
		icons_reader_c::icons_reader_c(const std::string& path)
			: m_path (bsw::utf8_to_wstring(path)),
			  m_data (nullptr),
			  m_size (0)
		{
			m_stream = new std::ifstream(path, std::ios::binary);
			_load_container();
		}
		// ------------------------------------------------------------------------
		icons_reader_c::icons_reader_c(const std::wstring& path)
			: m_path(path),
			  m_data(nullptr),
			  m_size(0)
		{
			m_stream = new std::ifstream(path, std::ios::binary);
			_load_container();
		}
		// ------------------------------------------------------------------------
		icons_reader_c::icons_reader_c(const char* data, std::size_t size)
			: m_stream (nullptr),
			m_data (data),
			m_size (size)
		{
			_load_container();
		}
		// -------------------------------------------------------------------------
		icons_reader_c::~icons_reader_c()
		{
			if (m_stream)
			{
				delete m_stream;
			}
		}
		// -------------------------------------------------------------------------
		void icons_reader_c::_load_container()
		{
			if (m_data)
			{
				try
				{
					pe_image_c i(m_data, m_size, m_container);
					return;
				}
				catch (...)
				{
					try
					{
						ne_image_c i(m_data, m_size, m_container);
						return;
					}
					catch (...)
					{
						try
						{
							bsw::istream_wrapper_c w(m_data, m_size);
							ico_image_loader(w, m_container);
							return;
						}
						catch (...)
						{
							
						}
					}
				}
			}
			else
			{
				try
				{
					pe_image_c i(m_path, m_container);
					return;
				}
				catch (...)
				{
					bsw::istream_wrapper_c w(*m_stream);
					try
					{
						ne_image_c i(w, m_container);
						return;
					}
					catch (...)
					{
						try
						{
							
							ico_image_loader(w, m_container);
							return;
						}
						catch (...)
						{

						}
					}
				}
			}
		}
		// -------------------------------------------------------------------------
		bool icons_reader_c::load_icon(std::vector <unsigned char>& out, unsigned& w, unsigned& h, std::streampos offs, uint32_t size)
		{
			if (m_stream)
			{
				bsw::istream_wrapper_c is(*m_stream);
				return spy::am::load_icon(out, w, h, offs, size, is);
			}
			else
			{
				bsw::istream_wrapper_c is(m_data, m_size);
				return spy::am::load_icon(out, w, h, offs, size, is);
			}
		}
		// -------------------------------------------------------------------------
		bool icons_reader_c::load_icon_dimensions(unsigned& w, unsigned& h, int& bpp, std::streampos offs, uint32_t size)
		{
			if (m_stream)
			{
				return spy::am::load_icon_dimensions(w, h, bpp, offs, size, *m_stream);
			}
			else
			{
				return spy::am::load_icon_dimensions(w, h, bpp, offs, size, m_data, m_size);
			}
		}
		// -------------------------------------------------------------------------------
		const icons_container_c& icons_reader_c::container() const
		{
			return m_container;
		}
	} // ns am
} // ns spy
