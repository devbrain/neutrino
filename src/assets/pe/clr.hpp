#ifndef __PEFILE_CLR_HPP__
#define __PEFILE_CLR_HPP__

#include "pefile.hpp"

namespace pefile
{
	struct CLR_HEADER
	{
		u4 Cb; //  Size of the header in bytes
		u2 MajorRuntimeVersion; //  The minimum version of the runtime required to run 	this program, currently 2.
		u2 MinorRuntimeVersion; //  The minor portion of the version, currently 0.
		DATA_DIRECTORY MetaData; //  RVA and size of the physical metadata.
		u4 Flags; // Flags describing this runtime image
		union // Token for the MethodDef or File of the entry point for the image
		{
			u4               EntryPointToken;
			u4               EntryPointRVA;
		}; 
		 
		DATA_DIRECTORY Resources; // RVA and size of implementation - specific resources.
		DATA_DIRECTORY StrongNameSignature; //  RVA of the hash data for this PE file used by the CLI loader for binding and versioning
		DATA_DIRECTORY CodeManagerTable; //  Always 0 
		DATA_DIRECTORY VTableFixups; //  RVA of an array of locations in the file that contain an array of function pointers(e.g., vtable slots).
		DATA_DIRECTORY ExportAddressTableJumps;//  Always 0 
		DATA_DIRECTORY ManagedNativeHeader; //  Always 0

		enum RUNTIME_FLAGS
		{
			COMIMAGE_FLAGS_ILONLY = 0x00000001, //Shall be 1.
			COMIMAGE_FLAGS_32BITREQUIRED = 0x00000002, //Image can only be loaded into a 32 - bit process,
			//for instance if there are 32 - bit vtablefixups, or
			//	casts from native integers to int32.CLI
			//	implementations that have 64 - bit native
			//	integers shall refuse loading binaries with this
			//	flag set.
			COMIMAGE_FLAGS_STRONGNAMESIGNED =  0x00000008, //Image has a strong name signature.
			COMIMAGE_FLAGS_NATIVE_ENTRYPOINT  = 0x00000010, // Shall be 0.
			COMIMAGE_FLAGS_TRACKDEBUGDATA = 0x00010000, 	// Other kinds of flags follow
		};
	};

	struct CLR_STREAM_HEADER
	{
		u4 Offset;
		u4 Size;
		std::string name;
	};

	struct CLR_METADATA
	{
		u4          offset_in_file;
		std::string version;
		std::vector <CLR_STREAM_HEADER> stream_headers;
	};
	// ----------------------------------------------------------------------
	struct CLR_METADATA_STREAM_HEADER
	{
		u4 Reserved_1;
		u1 MajorVersion;
		u1 MinorVersion;
		u1 HeapOffsetsizes;
		u1 Reserved_2;
		u8 MaskValid;
		u8 MaskSorted;
	};
	static_assert (sizeof(CLR_METADATA_STREAM_HEADER) == 24, "CLR_METADATA_STREAM_HEADER should be 24 bytes");
	// ----------------------------------------------------------------------
	using index_t = u4;
	struct CLR_TABLE_MODULE
	{
		u2      Generation;
		index_t Name;
		index_t Mvid;
		index_t Encid;
		index_t EncBaseId;
	};
	// ----------------------------------------------------------------------
	struct clr_resource_descriptor_s
	{
		uint32_t offset_in_file;
		uint32_t size;
		std::string name;
	};
	// ----------------------------------------------------------------------
	class clr_c
	{
	public:
		clr_c(file_c& file);

		void load_resources(std::vector <clr_resource_descriptor_s>& out);
		void read_resource(const clr_resource_descriptor_s& rd, std::vector <char>& out) const;
	private:
		void _parse_metadata();
		void _parse_metadata_stream();
		uint32_t _get_stream_offset(const CLR_STREAM_HEADER& s) const;
		void _load_internal_resources(uint32_t offset, uint32_t size, std::vector <clr_resource_descriptor_s>& out);
	private:
		file_c& file;
		const CLR_HEADER* clr_header;
		CLR_METADATA metadata;
		CLR_METADATA_STREAM_HEADER metadata_header;
		std::size_t metadata_stream_index;

		using table_name_size_t = std::tuple<uint16_t, uint32_t>;
		std::vector <table_name_size_t> table_sizes;
		std::size_t tables_offset;

	};
}

#endif
