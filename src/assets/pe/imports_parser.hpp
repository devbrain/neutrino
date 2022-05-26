#ifndef __PEFILE_IMPORTS_PARSER_HPP__
#define __PEFILE_IMPORTS_PARSER_HPP__

#include <map>
#include <iosfwd>
#include "pefile.hpp"


namespace pefile
{
	struct import_entry_s
	{
		import_entry_s(int ord);
		import_entry_s(const std::string& nm);

		std::string name;
		int ordinal;
	};

	std::ostream& operator << (std::ostream& os, const import_entry_s& x);

	using imports_table_t = std::multimap <std::string, import_entry_s>;
	void parse_imports(const file_c& pefile, imports_table_t& imports);
	std::size_t count_imports(const file_c& pefile);
}

#endif
