#include <iostream>
#include <sstream>
#include <stdexcept>

#include "pefile.hpp"
#include "resource_directory.hpp"
#include <neutrino/utils/strings/wchar.hh>

namespace pefile
{
	
	// ====================================================================
	resource_name_c::resource_name_c(int id)
		: m_value(std::make_pair(id, L""))
	{

	}
	// --------------------------------------------------------------------
	resource_name_c::resource_name_c(const std::wstring& name)
		: m_value(std::make_pair(-1, name))
	{

	}
	// --------------------------------------------------------------------
	bool resource_name_c::is_id() const
	{
		return m_value.second.empty();
	}
	// --------------------------------------------------------------------
	int resource_name_c::id() const
	{
		return m_value.first;
	}
	// --------------------------------------------------------------------
	void resource_name_c::id(int x)
	{
		m_value.first = x;
		m_value.second = L"";
	}
	// --------------------------------------------------------------------
	std::wstring resource_name_c::name() const
	{
		return m_value.second;
	}
	// --------------------------------------------------------------------
	void resource_name_c::name(const std::wstring& x)
	{
		m_value.first = -1;
		m_value.second = x;
	}
	// --------------------------------------------------------------------
	bool resource_name_c::is_special() const
	{
		if (is_id())
		{
			if (m_value.first >= CURSOR && m_value.first <= MANIFEST && m_value.first != 18)
			{
				return true;
			}
			switch (m_value.first)
			{
			case NEWBITMAP:
			case NEWMENU:
			case NEWDIALOG:
				return true;
			default:
				return false;
			}
		}
		return false;
	}
	// --------------------------------------------------------------------
	bool operator < (const resource_name_c& a, const resource_name_c& b)
	{
		return a.m_value < b.m_value;
	}
	// --------------------------------------------------------------------
	std::wostream& operator << (std::wostream& os, const resource_name_c& a)
	{
		if (a.is_id())
		{
			os << a.id();
		}
		else
		{
			os << a.name();
		}
		return os;
	}
	// --------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, const resource_name_c& a)
	{
		if (a.is_id())
		{
			os << a.id();
		}
		else
		{
			os << neutrino::utils::wstring_to_utf8(a.name());
		}
		return os;
	}
	// ============================================================================
	resource_c::resource_c()
		: m_language_code(0),
		m_offset(0),
		m_size(0)
	{

	}
	// -------------------------------------------------------------------------
	int resource_c::language_code() const
	{
		return m_language_code;
	}
	// -------------------------------------------------------------------------
	void resource_c::language_code(int x)
	{
		m_language_code = x;
	}
	// -------------------------------------------------------------------------
	uint32_t resource_c::offset() const
	{
		return m_offset;
	}
	// -------------------------------------------------------------------------
	void resource_c::offset(uint32_t x)
	{
		m_offset = x;
	}
	// -------------------------------------------------------------------------
	uint32_t resource_c::size() const
	{
		return m_size;
	}
	// -------------------------------------------------------------------------
	void resource_c::size(uint32_t x)
	{
		m_size = x;
	}
	const resource_name_c& resource_c::name() const
	{
		return m_rn;
	}
	// -------------------------------------------------------------------------
	void resource_c::name(resource_name_c& rn)
	{
		m_rn = rn;
	}
	// -------------------------------------------------------------------------
	std::size_t resource_c::offset_in_file(const file_c& file) const
	{
		const char* file_data = file.file_data();
		const std::size_t file_size = file.file_size();
		const auto& entry = file.optional_header().DataDirectory[(int)DataDirectory::Resource];
	//	const uint32_t rc_offs = file.translate_rva(entry.VirtualAddress);
		const uint32_t rc_offs = file.translate_rva(m_offset);

		uint64_t total = rc_offs;
//		total += m_offset;
		total += m_size;

		if (total >= file_size)
		{
			return file_size;
		}
		//return rc_offs + m_offset;
		return rc_offs;
	}
	// =======================================================================
	resource_dir_c::iterator resource_dir_c::begin(const resource_name_c& rn) const
	{
		auto i = m_dir.find(rn);
		if (i == m_dir.end())
		{
			std::ostringstream os;
			os << "No main section found for " << rn;
			throw std::runtime_error(os.str());
		}
		return i->second.begin();
	}
	// -------------------------------------------------------------------------
	resource_dir_c::iterator resource_dir_c::end(const resource_name_c& rn) const
	{
		auto i = m_dir.find(rn);
		if (i == m_dir.end())
		{
			std::ostringstream os;
			os << "No main section found for " << rn;
			throw std::runtime_error(os.str());
		}
		return i->second.end();
	}
	// -----------------------------------------------------------------------
	bool resource_dir_c::exists(int id) const
	{
		resource_name_c rn(id);
		return m_dir.find(rn) != m_dir.end();
	}
	// -----------------------------------------------------------------------
	resource_dir_c::names_iterator_c resource_dir_c::names_begin() const
	{
		return names_iterator_c(m_dir.begin());
	}
	// -----------------------------------------------------------------------
	resource_dir_c::names_iterator_c resource_dir_c::names_end() const
	{
		return names_iterator_c(m_dir.end());
	}
	// =======================================================================
	resource_dir_c::names_iterator_c& resource_dir_c::names_iterator_c::operator++ ()
	{
		m_itr++;
		return *this;
	}
	// -----------------------------------------------------------------------
	bool resource_dir_c::names_iterator_c::operator == (const names_iterator_c& a)
	{
		return this->m_itr == a.m_itr;
	}
	// -----------------------------------------------------------------------
	bool resource_dir_c::names_iterator_c::operator != (const names_iterator_c& a)
	{
		return !(*this == a);
	}
	// -----------------------------------------------------------------------
	const resource_name_c* resource_dir_c::names_iterator_c::operator -> () const
	{
		return &m_itr->first;
	}
	// -----------------------------------------------------------------------
	const resource_name_c& resource_dir_c::names_iterator_c::operator * () const
	{
		return m_itr->first;
	}
	// -----------------------------------------------------------------------
	resource_dir_c::names_iterator_c::names_iterator_c(itr_t i)
		: m_itr(i)
	{

	}
} // ns pefile
