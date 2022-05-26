#ifndef __PEFILE_RS_MANIFEST_HPP__
#define __PEFILE_RS_MANIFEST_HPP__

#include <string>
#include "resource_directory.hpp"
#include "pefile.hpp"


namespace pefile
{
	class manifest_c
	{
	public:
		static constexpr int resource_id()
		{
			return 24;
		}

		static constexpr bool singleton()
		{
			return true;
		}
	public:
		const std::string& text() const;

		static void load(const file_c& file, const resource_c& rn, manifest_c& out);
		void parse(std::string& name, std::string& description,
			std::string& level, std::string& ui);
	private:
		void _text(const char* t);
	public:
		std::string m_text;
		
	};
	// ===================================================
	
} // ns pefile

#endif
