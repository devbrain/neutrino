#include <sstream>
#include "rs_manifest.hpp"
//#include "bsw/xml/epp/xml_handler.hh"
//#include "bsw/xml/epp/xml.hh"
namespace
{
#if 0
	class manifest_parser_c
	{
	public:
		manifest_parser_c()
			: m_depth(0),
			m_in_descr(false)
		{

		}

		void start_element(const std::string& name, const bsw::xml::attributes& atts)
		{
			m_in_descr = false;
			m_depth++;
			if (name == "assemblyIdentity" && m_depth < 2)
			{
				m_name = atts.get("name");
			}
			else
			{
				if (name == "description")
				{
					m_in_descr = true;
				}
				else
				{
					if (name == "requestedExecutionLevel")
					{
						m_level = atts.get("level");
						m_ui = atts.get("uiAccess");
					}
				}
			}
		}

		void end_element(const std::string& name)
		{
			m_depth--;
			if (name == "description")
			{
				m_description = os.str();
				os.str("");
				os.clear();
			}
		}

		void character_data(const char* data, std::size_t length)
		{
			if (m_in_descr)
			{
				for (std::size_t i = 0; i < length; i++)
				{
					os << data[i];
				}
			}
		}
	public:
		std::string m_name;
		std::string m_description;
		std::string m_level;
		std::string m_ui;
	private:
		bool m_in_descr;
		int m_depth;
		std::ostringstream os;
	};
#endif
}
namespace pefile
{
	void manifest_c::_text(const char* t)
	{
		m_text = t;
	}
	// ------------------------------------------------------------------
	const std::string& manifest_c::text() const
	{
		return m_text;
	}
	// ------------------------------------------------------------------
	void manifest_c::load(const file_c& file, const resource_c& rn, manifest_c& out)
	{
		const char* file_data = file.file_data();
		const std::size_t file_size = file.file_size();
		auto offs = rn.offset_in_file(file);

		if (offs >= file_size)
		{
			return;
		}

		bsw::istream_wrapper_c stream(file_data + offs, rn.size());

		out.m_text.assign(stream.rd_ptr(), rn.size());
	}
	// ----------------------------------------------------------------
	void manifest_c::parse(std::string& name, std::string& description,
		std::string& level, std::string& ui)
	{
#if 0
		manifest_parser_c parser;
		try
		{
			bsw::xml::parse(parser, m_text);
			name = parser.m_name;
			description = parser.m_description;
			level = parser.m_level;
			ui = parser.m_ui;
		}
		catch (...)
		{

		}
#endif
	}
} // ns pefile
