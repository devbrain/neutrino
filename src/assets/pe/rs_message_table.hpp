#ifndef __PEFILE_RS_MESSAGE_TABLE_HPP__
#define __PEFILE_RS_MESSAGE_TABLE_HPP__

#include <map>
#include <string>
#include "resource_directory.hpp"
#include "pefile.hpp"

namespace pefile
{
	class message_table_c
	{
	public:
		static constexpr int resource_id()
		{
			return 11;
		}

		static constexpr bool singleton()
		{
			return false;
		}
	public:
		typedef std::map <uint16_t, std::wstring> msg_table_t;
	public:
		msg_table_t::const_iterator begin() const;
		msg_table_t::const_iterator end() const;

		bool exists(uint16_t key) const;
		std::wstring operator [] (uint16_t key) const;

		static void load(const file_c& file, const resource_c& rn, message_table_c& out);
	private:
		std::map <uint16_t, std::wstring> m_messages;
	};

} // ns pefile

#endif