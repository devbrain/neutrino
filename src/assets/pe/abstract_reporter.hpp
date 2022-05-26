#ifndef __PEFILE_ABSTRACT_REPORTER_HPP__
#define __PEFILE_ABSTRACT_REPORTER_HPP__

#include <stdint.h>
#include <iosfwd>

namespace pefile
{
	class abstract_reporter_c
	{
	public:
		enum header_field_t
		{
			COFF_HEADER_MACHINE_TYPE,
			COFF_HEADER_CHARACTERISTICS,

			OPTIONAL_HEADER_MAGIC,
			OPTIONAL_HEADER_IMAGE_BASE,
			OPTIONAL_HEADER_SUBSYSTEM,
			OPTIONAL_HEADER_DLL_CHARACTERISITCS,
			OPTIONAL_HEADER_SECTION_ALIGNMENT,
			OPTIONAL_HEADER_FILE_ALIGNMENT,
			OPTIONAL_HEADER_SIZE_OF_IMAGE
		};
		enum struct_name_t
		{
			OPTIONAL_HEADER
		};

		enum reserved_field_t
		{
			OPTIONAL_HEADER_WIN32VERSIONVALUE,
			OPTIONAL_HEADER_LOADER_FLAGS
		};
	public:
		virtual ~abstract_reporter_c();

		virtual void invalid_enum_value(header_field_t f, uint64_t actual_value);
		virtual void invalid_field_value(header_field_t f, uint64_t actual_value);
		virtual void invalid_struct_length(struct_name_t name, uint64_t expected, uint64_t actual);
		virtual void unknown_flag_set(header_field_t f, uint64_t actual_value);
		virtual void should_be_zero(reserved_field_t f, uint64_t actual_value);
		virtual void bad_checksum();
	};

	std::ostream& operator << (std::ostream& os, abstract_reporter_c::header_field_t f);
	std::ostream& operator << (std::ostream& os, abstract_reporter_c::struct_name_t f);
	std::ostream& operator << (std::ostream& os, abstract_reporter_c::reserved_field_t f);
} // ns pefile

#endif
