#ifndef __PEFILE_EXEFILE_HPP__
#define __PEFILE_EXEFILE_HPP__

#include <memory>

#include "pefile.hpp"
#include "imports_parser.hpp"
#include "resource_directory.hpp"
#include "rs_version.hpp"
#include "rs_manifest.hpp"
#include "rs_icon_group.hpp"
#include "rs_string_table.hpp"
#include "rs_dialog.hpp"
#include "rs_message_table.hpp"

#include "clr.hpp"

namespace pefile
{
	class exe_file_c
	{
	public:
		exe_file_c(const std::string& path);
		exe_file_c(const std::wstring& path);
		exe_file_c(const char* data, std::size_t size);

		bool compressed() const;
		const char* entry_point (std::size_t sz) const;
		bool is_64() const;
		bool is_gui() const;

		const imports_table_t& imports() const;
		void load_imports();
		std::size_t file_size() const;
		const char* file_data() const;
		std::size_t import_size() const;

		template <typename T>
		bool load_resource(T& out) const;
		
		template <typename T>
		bool load_resource(std::vector<T>& out) const;

		template <typename T>
		void load_resource(const resource_c& rn, T& out) const
		{
			T::load(*m_pefile.get(), rn, out);
		}

		std::vector <resource_name_c> resources() const;

		std::vector<SECTION> sections() const;
		float entropy(const SECTION& s) const;
		const char* read_section(const SECTION& s) const;
		const OPTIONAL_HEADER& optional_header() const;

		clr_c load_clr();

		std::size_t get_overlay_offset() const;
		const resource_dir_c& resource_directory() const;
		std::size_t resource_offset(const resource_c& rn) const;
	private:
		void _load();
	private:
		std::unique_ptr<file_c> m_pefile;
		bool m_is_gui;
		bool m_is_64_bit;
		resource_dir_c m_resource_directory;
		imports_table_t m_imports;
		uint32_t        m_entry_point;

	};

	template <typename T>
	bool exe_file_c::load_resource(T& out) const
	{
		auto ni = m_resource_directory.names_begin();
		auto end = m_resource_directory.names_end();
		bool found = false;
		for (auto i = ni; i != end; ++i)
		{
			if (i->is_id() && i->id() == T::resource_id())
			{
				for (auto j = m_resource_directory.begin(*i); j != m_resource_directory.end(*i); j++)
				{
					found = true;
					T::load(*m_pefile.get (), j->second, out);
				}
			}
		}
		return found;
	}
	// ---------------------------------------------------------------------
	template <typename T>
	bool exe_file_c::load_resource(std::vector<T>& out) const
	{
		auto ni = m_resource_directory.names_begin();
		auto end = m_resource_directory.names_end();
		for (auto i = ni; i != end; ++i)
		{
			if (i->is_id() && i->id() == T::resource_id())
			{
				for (auto j = m_resource_directory.begin(*i); j != m_resource_directory.end(*i); j++)
				{
					T v;
					T::load(*m_pefile.get(), j->second, v);
					out.push_back(v);
				}
			}
		}
		return !out.empty ();
	}
} // ns pefile

#endif
