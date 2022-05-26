#include <stdint.h>
#include <algorithm>
#include <iostream>

#include "imports_parser.hpp"
#include "istream_wrapper.hpp"

namespace pefile
{
	struct IMAGE_IMPORT_DESCRIPTOR
	{
		uint32_t   rvaImportLookupTable;
		uint32_t   TimeDateStamp;
		uint32_t   ForwarderChain;
		uint32_t   rvaModuleName;
		uint32_t   rvaImportAddressTable;
	};

	static_assert (sizeof(IMAGE_IMPORT_DESCRIPTOR) == 20, "IMAGE_IMPORT_DESCRIPTOR size should be 20 bytes");
	// -----------------------------------------------------------------------------------------
	static std::string load_name(const char* data, std::size_t size)
	{
		auto n = size;
		if (n > 64)
		{
			n = 64;
		}
		std::size_t end = 0;
		for (std::size_t i = 0; i < n; i++, end++)
		{
			auto ch = data[i];
			if (ch == 0)
			{
				break;
			}
		}
		return std::string(data, data + end);
	}
	// -----------------------------------------------------------------------------------------
	struct IMAGE_THUNK_DATA 
	{
		union 
		{
			uint32_t Ordinal;
			uint32_t AddressOfData;        
		} u;
	};
	// -----------------------------------------------------------------------------------------
	std::size_t count_imports(const file_c& pefile)
	{
		const char* file_data = pefile.file_data();
		const std::size_t file_size = pefile.file_size();
		const auto& entry = pefile.optional_header().DataDirectory[(int)DataDirectory::Import];
		const uint32_t iat_offs = pefile.translate_rva(entry.VirtualAddress);
		if (iat_offs == 0 || (iat_offs + entry.Size) > file_size)
		{
			return 0;
		}
		std::size_t count = 0;
		bsw::istream_wrapper_c stream(file_data + iat_offs, entry.Size);

		uint32_t has_bytes = 0;
		while (true)
		{
			auto descriptor = bsw::load_struct <IMAGE_IMPORT_DESCRIPTOR>(stream);

			if ((descriptor->rvaImportLookupTable == 0 && descriptor->rvaImportAddressTable == 0) || (descriptor->ForwarderChain == 0 &&
				descriptor->rvaModuleName == 0 && descriptor->TimeDateStamp == 0))
			{
				break;
			}
			const uint32_t module_name_offset = pefile.translate_rva(descriptor->rvaModuleName);
			if (module_name_offset < file_size)
			{
				uint32_t lookup = pefile.translate_rva(descriptor->rvaImportLookupTable);
				if (lookup >= iat_offs + entry.Size || lookup == 0)
				{
					lookup = pefile.translate_rva(descriptor->rvaImportAddressTable);
				}

				if (lookup < iat_offs + entry.Size && lookup > 0)
				{
					bsw::istream_wrapper_c thunk_stream(file_data + lookup, file_size - lookup);
					while (true)
					{
						auto thunk = bsw::load_struct <IMAGE_THUNK_DATA>(thunk_stream);
						if (thunk->u.Ordinal == 0)
						{
							break;
						}
						count++;
					}
				}

			}
			has_bytes += sizeof(IMAGE_IMPORT_DESCRIPTOR);
			if (has_bytes > entry.Size)
			{
				break;
			}
		}
		return count;
	}
	// -----------------------------------------------------------------------------------------
	void parse_imports(const file_c& pefile, imports_table_t& imports)
	{
		const char* file_data = pefile.file_data();
		const std::size_t file_size = pefile.file_size();
		const auto& entry = pefile.optional_header().DataDirectory[(int)DataDirectory::Import];
		const uint32_t iat_offs = pefile.translate_rva(entry.VirtualAddress);
		if (iat_offs == 0 || (iat_offs + entry.Size) > file_size)
		{
			return;
		}
		bsw::istream_wrapper_c stream(file_data + iat_offs, entry.Size);

		uint32_t has_bytes = 0;
		while (true)
		{
			auto descriptor = bsw::load_struct <IMAGE_IMPORT_DESCRIPTOR>(stream);

			if ((descriptor->rvaImportLookupTable == 0 && descriptor->rvaImportAddressTable == 0) || (descriptor->ForwarderChain == 0 && 
				descriptor->rvaModuleName == 0 	&&  descriptor->TimeDateStamp == 0))
			{
				break;
			}
			const uint32_t module_name_offset = pefile.translate_rva(descriptor->rvaModuleName);
			if (module_name_offset < file_size)
			{


				std::string name = load_name(file_data + module_name_offset, file_size - module_name_offset);

				uint32_t lookup = pefile.translate_rva(descriptor->rvaImportLookupTable);
				if (lookup >= iat_offs + entry.Size || lookup == 0)
				{
					lookup = pefile.translate_rva(descriptor->rvaImportAddressTable);
				}

				if (lookup < iat_offs + entry.Size && lookup > 0)
				{
					bsw::istream_wrapper_c thunk_stream(file_data + lookup, file_size - lookup);
					while (true)
					{
						auto thunk = bsw::load_struct <IMAGE_THUNK_DATA>(thunk_stream);
						if (thunk->u.Ordinal == 0)
						{
							break;
						}
						if (thunk->u.Ordinal & 0x80000000)
						{
							imports.insert(imports_table_t::value_type(name, import_entry_s(thunk->u.Ordinal & 0x7FFFFFFF)));
						}
						else
						{
							auto name_offs = 2 + pefile.translate_rva(thunk->u.Ordinal);
							imports.insert(imports_table_t::value_type(name, load_name(file_data + name_offs, file_size - name_offs)));
						}
					}
				}

			}
			has_bytes += sizeof(IMAGE_IMPORT_DESCRIPTOR);
			if (has_bytes > entry.Size)
			{
				break;
			}
		}
	}
	// ============================================================================
	import_entry_s::import_entry_s(int ord)
		: ordinal(ord),
		name("")
	{

	}
	// -----------------------------------------------------------------------------
	import_entry_s::import_entry_s(const std::string& nm)
		: ordinal (-1),
		name (nm)
	{

	}
	// -----------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, const import_entry_s& x)
	{
		if (x.ordinal != -1)
		{
			os << x.ordinal;
		}
		else
		{
			os << x.name;
		}
		return os;
	}
}