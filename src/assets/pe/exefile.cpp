#include <stdexcept>
#include <sstream>
#include <iostream>
#include "exefile.hpp"
#include "abstract_reporter.hpp"
#include "resource_builder.hpp"

namespace pefile
{
	namespace detail
	{
		class reporter_c : public abstract_reporter_c
		{
		public:
			void invalid_enum_value(header_field_t f, uint64_t actual_value) override;
			void invalid_field_value(header_field_t f, uint64_t actual_value) override;
			void invalid_struct_length(struct_name_t name, uint64_t expected, uint64_t actual) override;
			void unknown_flag_set(header_field_t f, uint64_t actual_value) override;
			void should_be_zero(reserved_field_t f, uint64_t actual_value) override;
			void bad_checksum() override;
		};

		void reporter_c::invalid_enum_value(header_field_t f, uint64_t actual_value)
		{
			std::ostringstream os;
			os << "Bad value for " << f << " 0x" << std::hex << actual_value;
			throw std::runtime_error(os.str());
		}
		// --------------------------------------------------------
		void reporter_c::invalid_field_value(header_field_t f, uint64_t actual_value)
		{
			std::ostringstream os;
			os << "Bad value for " << f << " 0x" << std::hex << actual_value;
			throw std::runtime_error(os.str());
		}
		// --------------------------------------------------------
		void reporter_c::unknown_flag_set(header_field_t f, uint64_t actual_value)
		{
			std::ostringstream os;
			os << "Unknown flag set in " << f << " 0x" << std::hex << actual_value;
			throw std::runtime_error(os.str());
		}
		// --------------------------------------------------------
		void reporter_c::invalid_struct_length(struct_name_t name, uint64_t expected, uint64_t actual)
		{
			std::ostringstream os;
			os << "Invalid " << name << " length. Expected " << expected << ", actual " << actual;
			throw std::runtime_error(os.str());
		}
		// --------------------------------------------------------
		void reporter_c::should_be_zero(reserved_field_t f, uint64_t actual_value)
		{
			std::ostringstream os;
			os << f << " should be zero, but it is " << actual_value;
			throw std::runtime_error(os.str());
		}
		// --------------------------------------------------------
		void reporter_c::bad_checksum()
		{
			throw std::runtime_error("Bad checksum");
		}
	} // ns detail
	// ============================================================================
	void exe_file_c::_load()
	{
		const auto& coff = m_pefile->coff_header();
		if (coff.Machine != COFF_HEADER::IMAGE_FILE_MACHINE_I386	 && coff.Machine != COFF_HEADER::IMAGE_FILE_MACHINE_AMD64)
		{
			std::ostringstream os;
			os << "Unsupported architecture " << static_cast <COFF_HEADER::IMAGE_FILE_MACHINE>(coff.Machine);
			throw std::logic_error(os.str());
		}
		if ((coff.Characteristics & COFF_HEADER::IMAGE_FILE_EXECUTABLE_IMAGE) != COFF_HEADER::IMAGE_FILE_EXECUTABLE_IMAGE)
		{
			throw std::logic_error("Corrupted image");
		}

		const auto& optional = m_pefile->optional_header();
		m_is_64_bit = optional.Is64Bit;
		m_is_gui = (optional.Subsystem == OPTIONAL_HEADER::IMAGE_SUBSYSTEM_WINDOWS_GUI);
		
		build_resources(*m_pefile.get(), m_resource_directory);
		m_entry_point = m_pefile->translate_rva(optional.AddressOfEntryPoint);
	}
	// ----------------------------------------------------------------------------
	void exe_file_c::load_imports()
	{
		pefile::parse_imports(*m_pefile.get(), m_imports);
	}
	// ----------------------------------------------------------------------------
	std::size_t exe_file_c::import_size() const
	{
		return pefile::count_imports(*m_pefile.get());
	}
	// ----------------------------------------------------------------------------
	exe_file_c::exe_file_c(const std::string& path)
	{
		static abstract_reporter_c reporter;
		m_pefile = std::make_unique<file_c>(path, reporter);
		_load();
	}
	// ---------------------------------------------------------------------------
	exe_file_c::exe_file_c(const std::wstring& path)
	{
		static abstract_reporter_c reporter;
		m_pefile = std::make_unique<file_c>(path, reporter);
		_load();
	}
	// ---------------------------------------------------------------------------
	exe_file_c::exe_file_c(const char* data, std::size_t size)
	{
		static abstract_reporter_c reporter;
		m_pefile = std::make_unique<file_c>(data, size, reporter);
		_load();
	}
	// ---------------------------------------------------------------------------
	const imports_table_t& exe_file_c::imports() const
	{
		return m_imports;
	}
	// ---------------------------------------------------------------------------
	std::vector <resource_name_c> exe_file_c::resources() const
	{
		std::vector <resource_name_c> out;
		auto ni = m_resource_directory.names_begin();
		auto end = m_resource_directory.names_end();
		bool found = false;
		for (auto i = ni; i != end; ++i)
		{
			out.push_back(*i);
		}
		return out;
	}
	// ---------------------------------------------------------------------------
	std::vector<SECTION> exe_file_c::sections() const
	{
		return m_pefile->sections();
	}
	// ---------------------------------------------------------------------------
	float exe_file_c::entropy(const SECTION& s) const
	{
		return m_pefile->entropy(s);
	}
	// ---------------------------------------------------------------------------
	const char* exe_file_c::read_section(const SECTION& s) const
	{
		return m_pefile->read_section(s);
	}
	// ---------------------------------------------------------------------------
	const OPTIONAL_HEADER& exe_file_c::optional_header() const
	{
		return m_pefile->optional_header();
	}
	// ---------------------------------------------------------------------------
	bool exe_file_c::compressed() const
	{
		auto E = m_pefile->entropy();
		if (E < 6.6)
		{
			return false;
		}
		
		return true;
	}
	// ---------------------------------------------------------------------------
	bool exe_file_c::is_64() const
	{
		return m_is_64_bit;
	}
	// ---------------------------------------------------------------------------
	bool exe_file_c::is_gui() const
	{
		return m_is_gui;
	}
	// ---------------------------------------------------------------------------
	std::size_t exe_file_c::file_size() const
	{
		return m_pefile->file_size();
	}
	// ---------------------------------------------------------------------------
	const char* exe_file_c::file_data() const
	{
		return m_pefile->file_data();
	}
	// ---------------------------------------------------------------------------
	const char* exe_file_c::entry_point(std::size_t sz) const
	{
		if (m_entry_point == 0)
		{
			return nullptr;
		}
		if (m_entry_point + sz < m_pefile->file_size())
		{
			return m_pefile->file_data() + m_entry_point;
		}
		return nullptr;
	}
	// ---------------------------------------------------------------------------
	clr_c exe_file_c::load_clr()
	{
		return clr_c(*m_pefile.get());
	}
	// ---------------------------------------------------------------------------
	std::size_t exe_file_c::get_overlay_offset() const
	{
		return m_pefile->overlay_offset();
	}
	// ---------------------------------------------------------------------------
	const resource_dir_c& exe_file_c::resource_directory() const
	{
		return m_resource_directory;
	}
	// ---------------------------------------------------------------------------
	std::size_t exe_file_c::resource_offset(const resource_c& rn) const
	{
		return rn.offset_in_file(*m_pefile.get());
	}
} // ns pefile
