#include <stdexcept>
#include <iostream>
#include <algorithm>

#include "clr.hpp"
#include "clr_tables.hpp"
#include "clr_res.hpp"
#include "istream_wrapper.hpp"
#include <neutrino/utils/strings/wchar.hh>


[[maybe_unused]] static unsigned count_bits(uint64_t value)
{
	unsigned int count = 0;
	while (value > 0) 
	{           // until all bits are zero
		if ((value & 1) == 1)
		{// check lower bit
			count++;
		}
		value >>= 1;              // shift bits, removing lower bit
	}
	return count;
}

namespace pefile
{
	clr_c::clr_c(file_c& f)
		: file(f),
		clr_header(nullptr)
	{
		auto clr_pos = f.optional_header().DataDirectory[(int)DataDirectory::CLR];
		if (clr_pos.VirtualAddress == 0 || clr_pos.Size < sizeof(CLR_HEADER))
		{
			throw std::logic_error("No CLR section found");
		}

		auto clr_off = f.translate_rva(clr_pos.VirtualAddress);
		const char* file_data = f.file_data();
		const std::size_t file_size = f.file_size();
		if (clr_off + clr_pos.Size >= file_size )
		{
			throw std::logic_error("CLR section is corrupted");
		}

		union
		{
			const char* bytes;
			const CLR_HEADER* hdr;
		} u;
		u.bytes = file_data + clr_off;
		clr_header = u.hdr;
		_parse_metadata();
		_parse_metadata_stream();
		std::vector <clr_resource_descriptor_s> out;
	}
	// ----------------------------------------------------------------
	void clr_c::_parse_metadata()
	{
		auto pos = clr_header->MetaData;
		if (pos.VirtualAddress == 0)
		{
			throw std::logic_error("No CLR/Metadata section found");
		}
		auto md_off = file.translate_rva(pos.VirtualAddress);
		const char* file_data = file.file_data();
		const std::size_t file_size = file.file_size();
		if (md_off + pos.Size >= file_size)
		{
			throw std::logic_error("CLR Metadata is corrupted");
		}
		bsw::istream_wrapper_c is(file_data + md_off, pos.Size);
		u4 Signature; // It's a simple DWORD-signature (similar to the ones you find in the DOS Header and the Optional Header). Anyway, the value of this signature has to be 0x424A5342.
		u2 MajorVersion, MinorVersion;// Two word elements that are totally ignored by the loader.The value is 0x0001 for both.
		u4 Reserved; // A DWORD, the value of which is always 0.
		u4 Length; //  The length of the UTF string that follows(it's the version string, something like: "v1.1.4322"). The length has to be rounded up to a multiple of 4.
		is >> Signature;
		if (Signature != 0x424A5342)
		{
			throw std::logic_error("wrong CLR Metadata signature");
		}
		is >> MajorVersion >> MinorVersion >> Reserved >> Length;
		if (Length == 0 || Length % 4 != 0)
		{
			throw std::logic_error("wrong CLR Metadata Length field");
		}
		const char* v = is.rd_ptr();
		
		if (v[Length - 1] == 0)
		{
			metadata.version = v;
		}
		else
		{
			metadata.version.assign(v, Length);
		}
		is.advance(Length);
		u2 Flags; // Reserved, this word is always 0.
		u2 Streams; // : A word telling us the number of streams present in the MetaData.
		is >> Flags >> Streams;
		
		metadata.offset_in_file = md_off;
		metadata_stream_index = Streams;
		for (u2 i = 0; i < Streams; i++)
		{
			CLR_STREAM_HEADER h;
			is >> h.Offset >> h.Size;
			h.name = is.rd_ptr();
			auto len = h.name.size() + 1; // count last zero
			auto rem = len % 4;
			if (rem > 0)
			{
				len += (4 - rem);
			} 
			is.advance(len);
			if (h.name == "#~")
			{
				metadata_stream_index = metadata.stream_headers.size();
			}
			metadata.stream_headers.push_back(h);
		}
	}
	// ----------------------------------------------------------------
	uint32_t clr_c::_get_stream_offset(const CLR_STREAM_HEADER& s) const
	{
		auto off = metadata.offset_in_file + s.Offset;
		if (off + s.Size > file.file_size())
		{
			throw std::logic_error(std::string("Corrupted stream ") + s.name);
		}
		return off;
	}
	// ----------------------------------------------------------------
	void clr_c::_parse_metadata_stream()
	{
		if (metadata_stream_index >= metadata.stream_headers.size())
		{
			throw std::logic_error("Metatdata stream is absent");
		}
		auto mds = metadata.stream_headers[metadata_stream_index];
		bsw::istream_wrapper_c is(file.file_data() + _get_stream_offset(mds), mds.Size);
		union
		{
			const char* bytes;
			const CLR_METADATA_STREAM_HEADER* header;
		} u;
		u.bytes = is.rd_ptr();
		is.advance(sizeof(CLR_METADATA_STREAM_HEADER));
		metadata_header = *u.header;

		uint64_t value = u.header->MaskValid;
		uint16_t count = 0;
		while (value > 0)
		{
			if ((value & 1) == 1)
			{
				uint32_t x;
				is >> x;
				table_name_size_t v(count, x);
				table_sizes.push_back(v);
			}
			value >>= 1;              // shift bits, removing lower bit
			count++;
		}
		tables_offset = is.rd_ptr() - file.file_data();
	}
	// -----------------------------------------------------------------------
	void clr_c::load_resources(std::vector <clr_resource_descriptor_s>& out)
	{
		auto mds = metadata.stream_headers[metadata_stream_index];
		bsw::istream_wrapper_c is(file.file_data() + tables_offset, mds.Size);
		for (const auto& v : table_sizes)
		{
			auto table_id = std::get<0>(v);
			auto rows = std::get<1>(v);
			if (table_id != CLR::ManifestResource::ECMA_ID)
			{
              [[maybe_unused]] auto offset = is.rd_ptr() - file.file_data();
				auto row_size = memory_size_at<CLR::TABLES>(table_id, metadata_header.HeapOffsetsizes);
				is.advance(row_size*rows);
			}
			else
			{
				std::vector <uint32_t> offsets;
				for (uint32_t r = 0; r < rows; r++)
				{
					CLR::ManifestResource::instance mft = read<CLR::ManifestResource>(metadata_header.HeapOffsetsizes, is);
					auto impl = std::get<CLR::ManifestResource::Implementation::impl_index()>(mft);
					auto offset = std::get<CLR::ManifestResource::Offset::impl_index()>(mft);
					if (impl == 0)
					{
						offsets.push_back(offset);
					}
				}
				std::sort(offsets.begin(), offsets.end());
				auto res_section_size = clr_header->Resources.Size;
				offsets.push_back(res_section_size);
				const auto n = offsets.size();
				for (std::size_t i = 0; i < n - 1; i++)
				{
					auto offset = offsets[i];
					auto size = offsets[i + 1] - offset;
					_load_internal_resources(offset, size, out);
				}

			}
		}
	}
	// -------------------------------------------------------------------------------------------
	void clr_c::_load_internal_resources(uint32_t offset, uint32_t size, std::vector <clr_resource_descriptor_s>& out)
	{
		auto pos = clr_header->Resources;
		if (pos.VirtualAddress == 0)
		{
			throw std::logic_error("No CLR/Resources section found");
		}
		auto md_off = file.translate_rva(pos.VirtualAddress);
        [[maybe_unused]] const char* file_data = file.file_data();
		const std::size_t file_size = file.file_size();
	 	if (md_off + offset>= file_size)
		{
			throw std::logic_error("CLR Resources section is corrupted");
		}
		bsw::istream_wrapper_c is(file.file_data() + md_off + offset, size);
		is.advance(4);
		
		clr_resource_c res;
		auto sz = is.size_to_end();
		if (0 == res.ProcessResourcesFile((const uint8_t*)is.rd_ptr(), static_cast<uint32_t>(sz)))
		{
			return;
		}
		static constexpr std::size_t NAME_LEN = 1024;
		wchar_t res_name[NAME_LEN];

		using resource_description_t = std::tuple <uint32_t, int, std::string>;
		std::vector<resource_description_t> resources;
        [[maybe_unused]] uint32_t data_offs = res.pDataSection - res.pBaseAddress;
		for (unsigned x = 0; x < res.NumberOfResources; x++)
		{
			uint32_t Offset = 0;
			int TypeIndex = 0;

			if (res.GetResourceInfo(x, res_name, 1024, &Offset, &TypeIndex))
			{
				if (Offset > 0)
				{
					for (std::size_t i = 0; i < NAME_LEN; i++)
					{
						if (res_name[i] == 0)
						{
							break;
						}
						else
						{
							if (res_name[i] == L'/')
							{
								res_name[i] = L'\\';
							}
						}
					}
					resource_description_t rd(Offset, TypeIndex, neutrino::utils::wstring_to_utf8(res_name));
					resources.push_back(rd);
				}
			}
		}
		resource_description_t sentinel(offset + size, -1, "");
		resources.push_back(sentinel);
		std::sort(resources.begin(), resources.end(), 
			[](const resource_description_t& a, const resource_description_t&b)
		{
			return std::get<0>(a) < std::get<0>(b);
		}
		);
		auto n = resources.size();
		for (std::size_t i = 0; i < n - 1; i++)
		{
			auto of   = std::get<0>(resources[i]) + 4;
			auto next = std::get<0>(resources[i+1]);
			uint32_t sz = next - of;
			clr_resource_descriptor_s rd;
			rd.name = std::get<2>(resources[i]);
			rd.offset_in_file = md_off + offset + 4 + of;
			rd.size = sz;
			out.push_back(rd);
		}
	}
	// ----------------------------------------------------------------------------------------------
	void clr_c::read_resource(const clr_resource_descriptor_s& rd, std::vector <char>& out) const
	{
		const char* file_data = file.file_data();
		const std::size_t file_size = file.file_size();
		if (rd.offset_in_file + rd.size >= file_size)
		{
			throw std::logic_error("CLR Resources description is corrupted");
		}
		out.resize(rd.size);
		std::memcpy(out.data(), file_data + rd.offset_in_file, rd.size);
	}
} // ns pefile
