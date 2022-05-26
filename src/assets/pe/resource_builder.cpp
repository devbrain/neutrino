#include "resource_builder.hpp"


namespace pefile
{
	namespace detail
	{
		class resource_dir_builder_c
		{
		public:
			resource_dir_builder_c(resource_dir_c& rd);

			void start_main_entry(const resource_name_c& rcname);
			void end_main_entry();

			void start_sub_entry(const resource_name_c& rcname);
			void add_to_sub_entry(resource_c& res);
			void end_sub_entry();
		private:
			using first_level_t = resource_dir_c::first_level_t;
			using second_level_t = resource_dir_c::second_level_t;
		private:
			first_level_t& m_dir;
			second_level_t m_curr;
			resource_name_c m_curr_main_entry;
			resource_name_c m_curr_sub_entry;
		};

		resource_dir_builder_c::resource_dir_builder_c(resource_dir_c& rd)
			: m_dir(rd.m_dir)
		{

		}
		// ------------------------------------------------------------------------
		void resource_dir_builder_c::start_main_entry(const resource_name_c& rcname)
		{
			m_curr.clear();
			m_curr_main_entry = rcname;
		}
		// -----------------------------------------------------------------------
		void resource_dir_builder_c::end_main_entry()
		{
			m_dir.insert(first_level_t::value_type(m_curr_main_entry, m_curr));
		}
		// -----------------------------------------------------------------------
		void resource_dir_builder_c::start_sub_entry(const resource_name_c& rcname)
		{
			m_curr_sub_entry = rcname;
		}
		// -----------------------------------------------------------------------
		void resource_dir_builder_c::add_to_sub_entry(resource_c& res)
		{
			res.name(m_curr_sub_entry);
			m_curr.insert(second_level_t::value_type(m_curr_sub_entry, res));
		}
		// -----------------------------------------------------------------------
		void resource_dir_builder_c::end_sub_entry()
		{

		}
		// =======================================================================
		struct IMAGE_RESOURCE_DIRECTORY_ENTRY
		{
			bool valid() const
			{
				return offset_info.OffsetToData != 0;
			}

			union {
				struct
				{
					uint32_t NameOffset : 31;
					uint32_t NameIsString : 1;
				} info;
				uint32_t   Name;
				uint16_t   Id;
			} name_info;
			union
			{
				uint32_t   OffsetToData;
				struct {
					uint32_t   OffsetToDirectory : 31;
					uint32_t   DataIsDirectory : 1;
				} info;
			} offset_info;
		};
		static_assert (sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) == 8, "sizeof of IMAGE_RESOURCE_DIRECTORY_ENTRY struct should be 8 bytes");
		// ======================================================================================
		struct IMAGE_RESOURCE_DIRECTORY
		{
			uint32_t Characteristics; // Theoretically this field could hold flags for the resource, but appears to always be 0.
			uint32_t TimeDateStamp; // 	The time / date stamp describing the creation time of the resource.
			// Theoretically these fields would hold a version number for the resource.These field appear to always be set to 0.
			uint16_t MajorVersion; 
			uint16_t MinorVersion; 
			uint16_t NumberOfNamedEntries; // The number of array elements that use names and that follow this structure.
			uint16_t NumberOfIdEntries; // The number of array elements that use integer IDs, and which follow this structure.
		};
		static_assert (sizeof(IMAGE_RESOURCE_DIRECTORY) == 16, "sizeof of IMAGE_RESOURCE_DIRECTORY struct should be 16 bytes");
		// ========================================================================
		uint32_t count_entries(bsw::istream_wrapper_c& is)
		{
			auto rd = bsw::load_struct <IMAGE_RESOURCE_DIRECTORY>(is);
			return rd->NumberOfIdEntries + rd->NumberOfNamedEntries;
		}
		// ------------------------------------------------------------------------
		std::wstring read_string(bsw::istream_wrapper_c& stream, uint32_t offs)
		{
			auto curr = stream.current_pos();
			stream.seek(offs);

			uint16_t size;
			stream >> size;

			std::wstring s;
			if (size < stream.size_to_end())
			{
				union
				{
					const char* bytes;
					const wchar_t* words;
				} u;
				u.bytes = stream.rd_ptr();
				s.assign(u.words, u.words + size);
			}

			stream.seek(curr);
			return s;
		}
		// ------------------------------------------------------------------------
		void parse_resource_directory(uint32_t rs_rva, uint32_t offs, bsw::istream_wrapper_c& stream, int depth, resource_dir_builder_c& rdb)
		{
			if (depth > 1)
			{
				std::cerr << "max depth reached" << std::endl;
				return;
			}
			
			stream.seek(offs);
			uint32_t count = count_entries(stream);
			for (uint32_t i = 0; i < count; i++)
			{
				auto entry = bsw::load_struct<IMAGE_RESOURCE_DIRECTORY_ENTRY>(stream);
				resource_name_c rname;
				if (depth == 0)
				{
					if (entry->name_info.info.NameIsString)
					{
						rname.name(read_string(stream, entry->name_info.info.NameOffset));
					}
					else
					{
						rname.id(entry->name_info.Id);
					}
				}
				if (entry->offset_info.info.DataIsDirectory)
				{
					if (depth == 0)
					{
						rdb.start_sub_entry(rname);
						auto pos = stream.current_pos();
						parse_resource_directory(rs_rva, entry->offset_info.info.OffsetToDirectory, stream, depth + 1, rdb);
						stream.seek(pos);
						rdb.end_sub_entry();
					}
				}
				else
				{
					if (depth == 1)
					{
						auto current = stream.current_pos();

						stream.seek(entry->offset_info.info.OffsetToDirectory);

						uint32_t data_rva;
						uint32_t size;

						stream >> data_rva;
						stream >> size;
						stream.seek(current);

						resource_c rc;
						rc.language_code(entry->name_info.Id);
					//	rc.offset(data_rva - rs_rva);
						rc.offset(data_rva);
						rc.size(size);
						rdb.add_to_sub_entry(rc);
					}
				}
			}
		}

		// ------------------------------------------------------------------------
		void create_res_dir(uint32_t rs_rva, bsw::istream_wrapper_c& stream, const file_c& pefile, resource_dir_builder_c& rdb)
		{
			const uint32_t count = count_entries(stream);
			for (uint32_t i = 0; i < count; i++)
			{
				auto entry = bsw::load_struct<IMAGE_RESOURCE_DIRECTORY_ENTRY>(stream);
				resource_name_c rname;
				if (entry->name_info.info.NameIsString)
				{
					rname.name(read_string(stream, entry->name_info.info.NameOffset));
				}
				else
				{
					rname.id(entry->name_info.Id);
				}
				if (entry->offset_info.info.DataIsDirectory)
				{
					rdb.start_main_entry(rname);
					auto pos = stream.current_pos();
					parse_resource_directory(rs_rva, entry->offset_info.info.OffsetToDirectory, stream, 0, rdb);
					stream.seek(pos);
					rdb.end_main_entry();
				}
			}
		}
	} // ns detail
	// ============================================================================
	void build_resources(const file_c& pefile, resource_dir_c& rd)
	{
		const char* file_data = pefile.file_data();
		const std::size_t file_size = pefile.file_size();
		const auto& entry = pefile.optional_header().DataDirectory[(int)DataDirectory::Resource];
		const uint32_t rc_offs = pefile.translate_rva(entry.VirtualAddress);
		if (rc_offs == 0 || (rc_offs + entry.Size) > file_size)
		{
			return;
		}
		bsw::istream_wrapper_c stream(file_data + rc_offs, entry.Size);
		detail::resource_dir_builder_c builder(rd);
		detail::create_res_dir(entry.VirtualAddress, stream, pefile, builder);
	}
} // ns pefile
