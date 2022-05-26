#include <iostream>
#include "abstract_reporter.hpp"

namespace pefile
{
	abstract_reporter_c::~abstract_reporter_c()
	{

	}
	// --------------------------------------------------------
	void abstract_reporter_c::invalid_enum_value(header_field_t f, uint64_t actual_value)
	{
		std::cout << "Bad value for " << f << " 0x" << std::hex << actual_value << std::dec << std::endl;
	}
	// --------------------------------------------------------
	void abstract_reporter_c::invalid_field_value(header_field_t f, uint64_t actual_value)
	{
		std::cout << "Bad value for " << f << " 0x" << std::hex << actual_value 
			<< std::dec << std::endl;
	}
	// --------------------------------------------------------
	void abstract_reporter_c::unknown_flag_set(header_field_t f, uint64_t actual_value)
	{
		std::cout << "Unknown flag set in " << f << " 0x" << std::hex << actual_value 
			<< std::dec << std::endl;
	}
	// --------------------------------------------------------
	void abstract_reporter_c::invalid_struct_length(struct_name_t name, uint64_t expected, uint64_t actual)
	{
		std::cout << "Invalid " << name << " length. Expected " << expected << ", actual " << actual << std::endl;
	}
	// --------------------------------------------------------
	void abstract_reporter_c::should_be_zero(reserved_field_t f, uint64_t actual_value)
	{
		std::cout << f << " should be zero, but it is " << actual_value << std::endl;
	}
	// --------------------------------------------------------
	void abstract_reporter_c::bad_checksum()
	{
		std::cout << "Bad checksum" << std::endl;
	}
	// ========================================================
	std::ostream& operator << (std::ostream& os, abstract_reporter_c::header_field_t f)
	{
		switch (f)
		{
		case abstract_reporter_c::COFF_HEADER_MACHINE_TYPE:
			os << "COFF_HEADER::Machine";
			break;
		case abstract_reporter_c::COFF_HEADER_CHARACTERISTICS:
			os << "COFF_HEADER::Characterisitcs";
			break;
		case abstract_reporter_c::OPTIONAL_HEADER_MAGIC:
			os << "OPTIONAL_HEADER::Magic";
			break;
		case abstract_reporter_c::OPTIONAL_HEADER_IMAGE_BASE:
			os << "OPTIONAL_HEADER::ImageBase";
			break;
		case abstract_reporter_c::OPTIONAL_HEADER_SUBSYSTEM:
			os << "OPTIONAL_HEADER::Subsystem";
			break;
		case abstract_reporter_c::OPTIONAL_HEADER_DLL_CHARACTERISITCS:
			os << "OPTIONAL_HEADER::DllCharacterisitcs";
			break;
		case abstract_reporter_c::OPTIONAL_HEADER_SECTION_ALIGNMENT:
			os << "OPTIONAL_HEADER::SectionAlignment";
			break;
		case abstract_reporter_c::OPTIONAL_HEADER_FILE_ALIGNMENT:
			os << "OPTIONAL_HEADER::FileAlignment";
			break;
		case abstract_reporter_c::OPTIONAL_HEADER_SIZE_OF_IMAGE:
			os << "OPTIONAL_HEADER::SizeOfImage";
			break;
		default:
			os << "--- UNKNOWN ---";
		}
		return os;
	}
	// ---------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, abstract_reporter_c::struct_name_t f)
	{
		switch (f)
		{
		case abstract_reporter_c::OPTIONAL_HEADER:
			os << "OPTIONAL_HEADER";
			break;
		default:
			os << "--- UNKNOWN ---";
		}
		return os;
	}
	// ------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, abstract_reporter_c::reserved_field_t f)
	{
		switch (f)
		{
		case abstract_reporter_c::OPTIONAL_HEADER_WIN32VERSIONVALUE:
			os << "OPTIONAL_HEADER::Win32VersionValue";
			break;
		case abstract_reporter_c::OPTIONAL_HEADER_LOADER_FLAGS:
			os << "OPTIONAL_HEADER::LoaderFlags";
			break;
		default:
			os << "--- UNKNOWN ---";
		}
		return os;

	}
} // ns pefile
