#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <ctime>
#include <cmath>

#include <neutrino/utils/strings/wchar.hh>

#include "pefile.hpp"
#include "abstract_reporter.hpp"

namespace pefile
{
	namespace detail
	{
		// ----------------------------------------------------------------------------
		file_container_c::file_container_c(const std::string& path)
		{
			m_mmf = std::make_unique <mio::mmap_source>(path.c_str());
			m_data = m_mmf->data();
			m_size = m_mmf->size();
		}
		// ----------------------------------------------------------------------------
		file_container_c::file_container_c(const std::wstring& path)
		{
			m_mmf = std::make_unique <mio::mmap_source>(neutrino::utils::wstring_to_utf8 (path));
			m_data = m_mmf->data();
			m_size = m_mmf->size();
		}
		// ----------------------------------------------------------------------------
		file_container_c::file_container_c(const char* fdata, std::size_t fsize)
		{
			m_data = fdata;
			m_size = fsize;
		}
		// ----------------------------------------------------------------------------
		std::size_t file_container_c::size() const
		{
			return m_size;
		}
		// ----------------------------------------------------------------------------
		const char* file_container_c::data() const
		{
			return m_data;
		}
	}
}

static bool isPowerOfTwo(unsigned int x)
{
	return (
		x == 1 || x == 2 || x == 4 || x == 8 || x == 16 || x == 32 ||
		x == 64 || x == 128 || x == 256 || x == 512 || x == 1024 ||
		x == 2048 || x == 4096 || x == 8192 || x == 16384 ||
		x == 32768 || x == 65536 || x == 131072 || x == 262144 ||
		x == 524288 || x == 1048576 || x == 2097152 ||
		x == 4194304 || x == 8388608 || x == 16777216 ||
		x == 33554432 || x == 67108864 || x == 134217728 ||
		x == 268435456 || x == 536870912 || x == 1073741824 ||
		x == 2147483648);
}
// ------------------------------------------------------------------------------
uint16_t ChkSum(uint16_t oldChk, const uint16_t * ptr, std::size_t len)
{
	uint32_t c = oldChk;
	while (len)
	{
		std::size_t l = std::min(len, (std::size_t)0x4000);
		len -= l;
		for (std::size_t j=0; j<l; ++j)
		{
			c += *ptr++;
		}
		c = (c & 0xffff) + (c>>16);
	}
	c = (c & 0xffff) + (c>>16);
	return (uint16_t)c;
}
// ----------------------------------------------------------------------------
static float CalculateEntropy(const char *pData, std::size_t nDataSize)
{
	float fEntropy = 0; // mb fEntropy=1.4426950408889634073599246810023;
	float bytes[256] = { 0.0 };
	float temp;
	const float ln2 = 1.0f / logf(2.0);
	for (std::size_t i = 0; i<nDataSize; i++)
	{
		bytes[(unsigned char)pData[i]] += 1;
	}

	for (int j = 0; j<256; j++)
	{
		temp = bytes[j] / (float)nDataSize;
		if (temp)
		{
			fEntropy += (-logf(temp))*bytes[j];
		}
	}

	fEntropy = ln2*fEntropy / (float)nDataSize;

	return fEntropy;
}
// ----------------------------------------------------------------------------
namespace pefile
{
	
	// -------------------------------------------------------------------
	static const uint16_t PE_IMAGE_OPTIONAL_HEADER_PE32 = 0x10B;
	static const uint16_t PE_IMAGE_OPTIONAL_HEADER_PE32_PLUS = 0x20B;

	struct OPTIONAL_HEADER_32
	{
		u2	Magic;                   //	The unsigned integer that identifies the state of the image file.The most common number is 0x10B, which identifies it as a normal executable file. 0x107 identifies it as a ROM image, and 0x20B identifies it as a PE32 + executable.
		u1	MajorLinkerVersion;      //	The linker major version number.
		u1	MinorLinkerVersion;	     // The linker minor version number.
		u4	SizeOfCode;	             // The size of the code(text) section, or the sum of all code sections if there are multiple sections.
		u4	SizeOfInitializedData;   //	The size of the initialized data section, or the sum of all such sections if there are multiple data sections.
		u4	SizeOfUninitializedData; //	The size of the uninitialized data section(BSS), or the sum of all such sections if there are multiple BSS sections.
		u4	AddressOfEntryPoint;     //	The address of the entry point relative to the image base when the executable file is loaded into memory.For program images, this is the starting address.For device drivers, this is the address of the initialization function.An entry point is optional for DLLs.When no entry point is present, this field must be zero.
		u4	BaseOfCode;              //	The address that is relative to the image base of the beginning-of-code section when it is loaded into memory.
		u4  BaseOfData;              // The address that is relative to the image base of the beginning-of-data section when it is loaded into memory.

		u4 	ImageBase;                   //	The preferred address of the first byte of image when loaded into memory; must be a multiple of 64�K.The default for DLLs is 0x10000000. The default for Windows CE EXEs is 0x00010000. The default for Windows�NT, Windows 2000, Windows�XP, Windows�95, Windows�98, and Windows�Me is 0x00400000.
		u4	SectionAlignment;            //	The alignment(in bytes) of sections when they are loaded into memory.It must be greater than or equal to FileAlignment.The default is the page size for the architecture.
		u4	FileAlignment;               //	The alignment factor(in bytes) that is used to align the raw data of sections in the image file.The value should be a power of 2 between 512 and 64�K, inclusive.The default is 512. If the SectionAlignment is less than the architecture�s page size, then FileAlignment must match SectionAlignment.
		u2	MajorOperatingSystemVersion; //	The major version number of the required operating system.
		u2	MinorOperatingSystemVersion; //	The minor version number of the required operating system.
		u2	MajorImageVersion;           //	The major version number of the image.
		u2	MinorImageVersion;           //	The minor version number of the image.
		u2	MajorSubsystemVersion;       //	The major version number of the subsystem.
		u2	MinorSubsystemVersion;       //	The minor version number of the subsystem.
		u4	Win32VersionValue;           //	Reserved, must be zero.
		u4	SizeOfImage;                 //	The size(in bytes) of the image, including all headers, as the image is loaded in memory.It must be a multiple of SectionAlignment.
		u4	SizeOfHeaders;               //	The combined size of an MSDOS stub, PE header, and section headers rounded up to a multiple of FileAlignment.
		u4	CheckSum;                    //	The image file checksum.The algorithm for computing the checksum is incorporated into IMAGHELP.DLL.The following are checked for validation at load time : all drivers, any DLL loaded at boot time, and any DLL that is loaded into a critical Windows process.
		u2	Subsystem;	                 // The subsystem that is required to run this image. see enum PE_IMAGE_SUBSYSTEM
		u2	DllCharacteristics;          //	For more information, see enum PE_IMAGE_DLLCHARACTERISTICS.
		u4	SizeOfStackReserve;	         // The size of the stack to reserve.Only SizeOfStackCommit is committed; the rest is made available one page at a time until the reserve size is reached.
		u4	SizeOfStackCommit;           //	The size of the stack to commit.
		u4	SizeOfHeapReserve;           //	The size of the local heap space to reserve.Only SizeOfHeapCommit is committed; the rest is made available one page at a time until the reserve size is reached.
		u4	SizeOfHeapCommit;            //	The size of the local heap space to commit.
		u4	LoaderFlags;                 //	Reserved, must be zero.
		u4	NumberOfRvaAndSizes;         //	The number of data - directory entries in the remainder of the optional header.Each describes a location and size.
	};

	static_assert (sizeof(OPTIONAL_HEADER_32) == 96, "OPTIONAL_HEADER_32 size should be 96 bytes");
	// ======================================================================================
	struct OPTIONAL_HEADER_64
	{
		u2	Magic;                   //	The unsigned integer that identifies the state of the image file.The most common number is 0x10B, which identifies it as a normal executable file. 0x107 identifies it as a ROM image, and 0x20B identifies it as a PE32 + executable.
		u1	MajorLinkerVersion;      //	The linker major version number.
		u1	MinorLinkerVersion;	     // The linker minor version number.
		u4	SizeOfCode;	             // The size of the code(text) section, or the sum of all code sections if there are multiple sections.
		u4	SizeOfInitializedData;   //	The size of the initialized data section, or the sum of all such sections if there are multiple data sections.
		u4	SizeOfUninitializedData; //	The size of the uninitialized data section(BSS), or the sum of all such sections if there are multiple BSS sections.
		u4	AddressOfEntryPoint;     //	The address of the entry point relative to the image base when the executable file is loaded into memory.For program images, this is the starting address.For device drivers, this is the address of the initialization function.An entry point is optional for DLLs.When no entry point is present, this field must be zero.
		u4	BaseOfCode;              //	The address that is relative to the image base of the beginning - of - code section when it is loaded into memory.

		u8 	ImageBase;                   //	The preferred address of the first byte of image when loaded into memory; must be a multiple of 64�K.The default for DLLs is 0x10000000. The default for Windows CE EXEs is 0x00010000. The default for Windows�NT, Windows 2000, Windows�XP, Windows�95, Windows�98, and Windows�Me is 0x00400000.
		u4	SectionAlignment;            //	The alignment(in bytes) of sections when they are loaded into memory.It must be greater than or equal to FileAlignment.The default is the page size for the architecture.
		u4	FileAlignment;               //	The alignment factor(in bytes) that is used to align the raw data of sections in the image file.The value should be a power of 2 between 512 and 64�K, inclusive.The default is 512. If the SectionAlignment is less than the architecture�s page size, then FileAlignment must match SectionAlignment.
		u2	MajorOperatingSystemVersion; //	The major version number of the required operating system.
		u2	MinorOperatingSystemVersion; //	The minor version number of the required operating system.
		u2	MajorImageVersion;           //	The major version number of the image.
		u2	MinorImageVersion;           //	The minor version number of the image.
		u2	MajorSubsystemVersion;       //	The major version number of the subsystem.
		u2	MinorSubsystemVersion;       //	The minor version number of the subsystem.
		u4	Win32VersionValue;           //	Reserved, must be zero.
		u4	SizeOfImage;                 //	The size(in bytes) of the image, including all headers, as the image is loaded in memory.It must be a multiple of SectionAlignment.
		u4	SizeOfHeaders;               //	The combined size of an MSDOS stub, PE header, and section headers rounded up to a multiple of FileAlignment.
		u4	CheckSum;                    //	The image file checksum.The algorithm for computing the checksum is incorporated into IMAGHELP.DLL.The following are checked for validation at load time : all drivers, any DLL loaded at boot time, and any DLL that is loaded into a critical Windows process.
		u2	Subsystem;	                 // The subsystem that is required to run this image.For more information, see �Windows Subsystem� later in this specification.
		u2	DllCharacteristics;          //	For more information, see �DLL Characteristics� later in this specification.
		u8	SizeOfStackReserve;	         // The size of the stack to reserve.Only SizeOfStackCommit is committed; the rest is made available one page at a time until the reserve size is reached.
		u8	SizeOfStackCommit;           //	The size of the stack to commit.
		u8	SizeOfHeapReserve;           //	The size of the local heap space to reserve.Only SizeOfHeapCommit is committed; the rest is made available one page at a time until the reserve size is reached.
		u8	SizeOfHeapCommit;            //	The size of the local heap space to commit.
		u4	LoaderFlags;                 //	Reserved, must be zero.
		u4	NumberOfRvaAndSizes;         //	The number of data - directory entries in the remainder of the optional header.Each describes a location and size.
	};
	static_assert (sizeof(OPTIONAL_HEADER_64) == 112, "OPTIONAL_HEADER_64 size should be 112 bytes");
	// ===========================================================================
	static void assign(OPTIONAL_HEADER& opt, const OPTIONAL_HEADER_32* x)
	{
		opt.Is64Bit = false;

		opt.MajorLinkerVersion = x->MajorLinkerVersion;
		opt.MinorLinkerVersion = x->MinorLinkerVersion;
		opt.SizeOfCode = x->SizeOfCode;
		opt.SizeOfInitializedData = x->SizeOfInitializedData;
		opt.SizeOfUninitializedData = x->SizeOfUninitializedData;
		opt.AddressOfEntryPoint = x->AddressOfEntryPoint;
		opt.BaseOfCode = x->BaseOfCode;
		opt.BaseOfData = x->BaseOfData;
		opt.ImageBase = x->ImageBase;
		opt.SectionAlignment = x->SectionAlignment;
		opt.FileAlignment = x->FileAlignment;
		opt.MajorOperatingSystemVersion = x->MajorOperatingSystemVersion;
		opt.MinorOperatingSystemVersion = x->MinorOperatingSystemVersion;
		opt.MajorImageVersion = x->MajorImageVersion;
		opt.MinorImageVersion = x->MinorImageVersion;
		opt.MajorSubsystemVersion = x->MajorSubsystemVersion;
		opt.MinorSubsystemVersion = x->MinorSubsystemVersion;
		opt.Win32VersionValue = x->Win32VersionValue;
		opt.SizeOfImage = x->SizeOfImage;
		opt.SizeOfHeaders = x->SizeOfHeaders;
		opt.CheckSum = x->CheckSum;
		opt.Subsystem = x->Subsystem;
		opt.DllCharacteristics = x->DllCharacteristics;
		opt.SizeOfStackReserve = x->SizeOfStackReserve;
		opt.SizeOfStackCommit = x->SizeOfStackCommit;
		opt.SizeOfHeapReserve = x->SizeOfHeapReserve;
		opt.SizeOfHeapCommit = x->SizeOfHeapCommit;
		opt.LoaderFlags = x->LoaderFlags;
		opt.NumberOfRvaAndSizes = x->NumberOfRvaAndSizes;

	}

	static void assign(OPTIONAL_HEADER& opt, const OPTIONAL_HEADER_64* x)
	{
		opt.Is64Bit = true;

		opt.MajorLinkerVersion = x->MajorLinkerVersion;
		opt.MinorLinkerVersion = x->MinorLinkerVersion;
		opt.SizeOfCode = x->SizeOfCode;
		opt.SizeOfInitializedData = x->SizeOfInitializedData;
		opt.SizeOfUninitializedData = x->SizeOfUninitializedData;
		opt.AddressOfEntryPoint = x->AddressOfEntryPoint;
		opt.BaseOfCode = x->BaseOfCode;
		opt.BaseOfData = 0xFFFFFFFF;
		opt.ImageBase = x->ImageBase;
		opt.SectionAlignment = x->SectionAlignment;
		opt.FileAlignment = x->FileAlignment;
		opt.MajorOperatingSystemVersion = x->MajorOperatingSystemVersion;
		opt.MinorOperatingSystemVersion = x->MinorOperatingSystemVersion;
		opt.MajorImageVersion = x->MajorImageVersion;
		opt.MinorImageVersion = x->MinorImageVersion;
		opt.MajorSubsystemVersion = x->MajorSubsystemVersion;
		opt.MinorSubsystemVersion = x->MinorSubsystemVersion;
		opt.Win32VersionValue = x->Win32VersionValue;
		opt.SizeOfImage = x->SizeOfImage;
		opt.SizeOfHeaders = x->SizeOfHeaders;
		opt.CheckSum = x->CheckSum;
		opt.Subsystem = x->Subsystem;
		opt.DllCharacteristics = x->DllCharacteristics;
		opt.SizeOfStackReserve = x->SizeOfStackReserve;
		opt.SizeOfStackCommit = x->SizeOfStackCommit;
		opt.SizeOfHeapReserve = x->SizeOfHeapReserve;
		opt.SizeOfHeapCommit = x->SizeOfHeapCommit;
		opt.LoaderFlags = x->LoaderFlags;
		opt.NumberOfRvaAndSizes = x->NumberOfRvaAndSizes;

	}
	// ===========================================================================
	template <typename T>
	struct flag_traits;
#define BEGIN_FLAG_TRAITS(TYPE) \
	template <> struct flag_traits <TYPE>\
	{\
	static constexpr uint32_t data[] = 

#define END_FLAG_TRAITS \
;\
	static constexpr std::size_t len()\
	{\
		return sizeof(data) / sizeof(uint32_t);\
	}\
	}
	// ===========================================================================
	BEGIN_FLAG_TRAITS(COFF_HEADER::PE_IMAGE_FILE_MACHINE) {
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_UNKNOWN,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_AM33,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_AMD64,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_ARM,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_ARMNT,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_EBC,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_I386,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_IA64,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_M32R,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_MIPS16,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_MIPSFPU,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_MIPSFPU16,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_POWERPC,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_POWERPCFP,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_R4000,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_RISCV32,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_RISCV64,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_RISCV128,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_SH3,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_SH3DSP,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_SH4,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_SH5,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_THUMB,
			COFF_HEADER::PE_IMAGE_FILE_MACHINE_WCEMIPSV2
	}END_FLAG_TRAITS;

	BEGIN_FLAG_TRAITS(COFF_HEADER::PE_IMAGE_FILE_CHARACTERISTICS) {
		COFF_HEADER::PE_IMAGE_FILE_RELOCS_STRIPPED,
			COFF_HEADER::PE_IMAGE_FILE_EXECUTABLE_IMAGE,
			COFF_HEADER::PE_IMAGE_FILE_LINE_NUMS_STRIPPED,
			COFF_HEADER::PE_IMAGE_FILE_LOCAL_SYMS_STRIPPED,
			COFF_HEADER::PE_IMAGE_FILE_AGGRESSIVE_WS_TRIM,
			COFF_HEADER::PE_IMAGE_FILE_LARGE_ADDRESS_AWARE,
			COFF_HEADER::PE_IMAGE_FILE_RESERVED,
			COFF_HEADER::PE_IMAGE_FILE_BYTES_REVERSED_LO,
			COFF_HEADER::PE_IMAGE_FILE_32BIT_MACHINE,
			COFF_HEADER::PE_IMAGE_FILE_DEBUG_STRIPPED,
			COFF_HEADER::PE_IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP,
			COFF_HEADER::PE_IMAGE_FILE_NET_RUN_FROM_SWAP,
			COFF_HEADER::PE_IMAGE_FILE_SYSTEM,
			COFF_HEADER::PE_IMAGE_FILE_DLL,
			COFF_HEADER::PE_IMAGE_FILE_UP_SYSTEM_ONLY,
			COFF_HEADER::PE_IMAGE_FILE_BYTES_REVERSED_HI
	} END_FLAG_TRAITS;


	BEGIN_FLAG_TRAITS(OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS) {
		OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_RESERVED1,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_RESERVED2,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_RESERVED3,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_RESERVED4,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_NX_COMPAT,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_NO_ISOLATION,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_NO_SEH,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_NO_BIND,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_APPCONTAINER,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_WDM_DRIVER,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_GUARD_CF,
			OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE
	} END_FLAG_TRAITS;

	BEGIN_FLAG_TRAITS(OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM) {
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_UNKNOWN,
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_NATIVE,
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_WINDOWS_GUI,
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_WINDOWS_CUI,
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_POSIX_CUI,
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_WINDOWS_CE_GUI,
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_EFI_APPLICATION,
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER,
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER,
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_EFI_ROM,
			OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_XBOX
	} END_FLAG_TRAITS;

	BEGIN_FLAG_TRAITS(SECTION::CHARACTERISITICS) {
			SECTION::PE_IMAGE_SCN_TYPE_NO_PAD,
			SECTION::PE_IMAGE_SCN_CNT_CODE,
			SECTION::PE_IMAGE_SCN_CNT_INITIALIZED_DATA,
			SECTION::PE_IMAGE_SCN_CNT_UNINITIALIZED_DATA,
			SECTION::PE_IMAGE_SCN_LNK_OTHER,
			SECTION::PE_IMAGE_SCN_LNK_INFO,
			SECTION::PE_IMAGE_SCN_LNK_REMOVE,
			SECTION::PE_IMAGE_SCN_LNK_COMDAT,
			SECTION::PE_IMAGE_SCN_GPREL,
			SECTION::PE_IMAGE_SCN_ALIGN_1BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_2BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_4BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_8BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_16BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_32BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_64BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_128BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_256BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_512BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_1024BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_2048BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_4096BYTES,
			SECTION::PE_IMAGE_SCN_ALIGN_8192BYTES,
			SECTION::PE_IMAGE_SCN_LNK_NRELOC_OVFL,
			SECTION::PE_IMAGE_SCN_MEM_DISCARDABLE,
			SECTION::PE_IMAGE_SCN_MEM_NOT_CACHED,
			SECTION::PE_IMAGE_SCN_MEM_NOT_PAGED,
			SECTION::PE_IMAGE_SCN_MEM_SHARED,
			SECTION::PE_IMAGE_SCN_MEM_EXECUTE,
			SECTION::PE_IMAGE_SCN_MEM_READ,
			static_cast <u4>(SECTION::PE_IMAGE_SCN_MEM_WRITE)
	} END_FLAG_TRAITS;

	template <typename T>
	bool check_in_enum(uint32_t v)
	{
		for (std::size_t i = 0; i < flag_traits<T>::len(); i++)
		{
			if (v == flag_traits<T>::data[i])
			{
				return true;
			}
		}
		return false;
	}
	// ===========================================================================
	template <typename T>
	bool check_in_flags(uint32_t v)
	{
		for (std::size_t i = 0; i < flag_traits<T>::len(); i++)
		{
			const uint32_t mask = ~flag_traits<T>::data[i];
			v = v & mask;
		}
		return (v == 0);
	}
	// ===========================================================================
	void file_c::_load(abstract_reporter_c& reporter)
	{
		if (!m_mmf.data())
		{
			throw std::runtime_error("Failed to open file");
		}
		bsw::istream_wrapper_c stream(m_mmf.data(), static_cast <std::size_t>(m_mmf.size()));

		auto fsize = m_mmf.size();
		static const uint16_t PE_IMAGE_DOS_SIGNATURE = 0x5A4D;
		static const uint32_t PE_IMAGE_NT_SIGNATURE = 0x00004550;
		uint16_t old_dos_magic;
		stream >> old_dos_magic;
		if (old_dos_magic != PE_IMAGE_DOS_SIGNATURE)
		{
			throw std::runtime_error("Not a MZ file");
		}
		stream.advance(26 + 32);
		int32_t lfanew;
		stream >> lfanew;
		if (lfanew < 0 || lfanew > fsize)
		{
			throw std::runtime_error("Not a PE file");
		}
		// read coff magic
		stream.seek(lfanew);
		uint32_t pe_magic;
		stream >> pe_magic;
		if (pe_magic != PE_IMAGE_NT_SIGNATURE)
		{
			throw std::runtime_error("Not a PE file");
		}
		_load_headers(stream, reporter);
		_load_sections(stream, reporter);
	}
	// ----------------------------------------------------------------------------
	file_c::file_c(const std::string& path, abstract_reporter_c& reporter)
		: m_mmf(path.c_str()),
		m_coff_header(nullptr)
	{
		_load(reporter);
	}
	// -------------------------------------------------------------
	file_c::file_c(const std::wstring& path, abstract_reporter_c& reporter)
		: m_mmf(path.c_str()),
		m_coff_header(nullptr)
	{
		_load(reporter);
	}
	// -------------------------------------------------------------
	file_c::file_c(const char* fdata, std::size_t fsize, abstract_reporter_c& reporter)
		: m_mmf(fdata, fsize),
		m_coff_header(nullptr)
	{
		_load(reporter);
	}
	// -------------------------------------------------------------
	bool file_c::checksum() const
	{
		// eval checksum
		union
		{
			const uint16_t* words;
			const char* bytes;
		} u;
		u.bytes = m_mmf.data();
		auto fsize = m_mmf.size();
		auto len = static_cast<std::size_t> (fsize / sizeof(uint16_t));
		uint32_t chk = ChkSum(0, u.words, len);
		if (fsize % sizeof(uint16_t))
		{
			chk += u.bytes[fsize - 1];
			chk = (chk >> 16) + (chk & 0xffff);
		}
		uint32_t yy = 0;
		if (chk - 1 < m_optional_header.CheckSum)
		{
			yy = (chk - 1) - m_optional_header.CheckSum;
		}
		else
		{
			yy = chk - m_optional_header.CheckSum;
		}
		yy = (yy & 0xffff) + (yy >> 16);
		yy = (yy & 0xffff) + (yy >> 16);
		uint32_t dwSize = static_cast<std::size_t> (fsize);
		yy += dwSize;
		return (yy == m_optional_header.CheckSum);
	}
	// -------------------------------------------------------------
	float file_c::entropy(const SECTION& section) const
	{
		const uint32_t offs = section.PointerToRawData;
		if (offs == 0)
		{
			return 0;
		}
		const uint32_t size = section.SizeOfRawData;
		if (size == 0)
		{
			return 0;
		}

		return CalculateEntropy(m_mmf.data() + offs, size);
	}
	// -------------------------------------------------------------
	const char* file_c::read_section(const SECTION& s) const
	{
		const char* file_data = this->file_data();
		const std::size_t file_size = this->file_size();
		const uint32_t rc_offs = translate_rva(s.VirtualAddress);
		
		if (rc_offs == 0 || (rc_offs + s.SizeOfRawData) > file_size)
		{
			return nullptr;
		}
		return file_data + rc_offs;
	}
	// -------------------------------------------------------------
	float file_c::entropy() const
	{
		return  CalculateEntropy(m_mmf.data(), static_cast<std::size_t>(m_mmf.size()));
#if 0
		float e = 0;
		float total = 0;
		for (const auto& sec : m_sections)
		{
			const uint32_t offs = sec.PointerToRawData;
			if (offs == 0)
			{
				continue;
			}
			const uint32_t size = sec.SizeOfRawData;
			if (size == 0)
			{
				continue;
			}

			auto es = CalculateEntropy(m_mmf.data() + offs, size) * (float)size;
			total = total + size;
			e = e + es;
		}

		auto E = e / total;

		return E;
#endif
	}
	// -------------------------------------------------------------
	file_c::~file_c()
	{

	}
	// --------------------------------------------------------------
	const char* file_c::file_data() const
	{
		return m_mmf.data();
	}
	// --------------------------------------------------------------
	std::size_t file_c::file_size() const
	{
		return static_cast<std::size_t>(m_mmf.size());
	}
	// --------------------------------------------------------------
	std::size_t file_c::overlay_offset() const
	{
		const section_t& table = sections();
		const OPTIONAL_HEADER& opt = optional_header();
		std::size_t offset = 0;
		if (table.empty())
		{
			offset = file_size();
		}
		for (const SECTION& section : table)
		{
			u4 aligned_pointer = section.PointerToRawData  & ~0x1ff;
			
			auto endPoint = _get_read_size(section) + aligned_pointer;
			offset = std::max(offset, (std::size_t)endPoint);
		}
		if (offset > file_size()) 
		{
			offset = file_size();
		}
		return offset;
	}
	// --------------------------------------------------------------
	static u4 alignedUp(u4 value, u4 alignTo) 
	{
		if (value % alignTo != 0) 
		{
			value = (value + alignTo - 1) & ~(alignTo - 1);
		}
		return value;
	}
	// --------------------------------------------------------------
	u4 file_c::_get_read_size(const SECTION& sec) const
	{
		auto pointerToRaw = sec.PointerToRawData;
		auto virtSize = sec.VirtualSize;
		auto sizeOfRaw = sec.SizeOfRawData;
		auto fileAlign = optional_header().FileAlignment;
		auto alignedPointerToRaw = pointerToRaw & ~0x1ff;
		auto readSize = alignedUp(pointerToRaw + sizeOfRaw, fileAlign) - alignedPointerToRaw;
		readSize = std::min(readSize, alignedUp(sizeOfRaw, 0x1000));
		if (virtSize != 0) 
		{
			readSize = std::min(readSize, alignedUp(virtSize, 0x1000));
		}
		return readSize;
	}
	// --------------------------------------------------------------
	uint32_t file_c::translate_rva(uint32_t rva) const
	{
		for (const auto& s : m_sections)
		{
			uint32_t size = s.VirtualSize ? s.VirtualSize : s.SizeOfRawData;
			if (rva >= s.VirtualAddress && rva < s.VirtualAddress + size)
			{
				return s.PointerToRawData + (rva - s.VirtualAddress);
			}
		}
		return 0;
	}
	// --------------------------------------------------------------
	void file_c::_load_headers(bsw::istream_wrapper_c& is, abstract_reporter_c& reporter)
	{

		m_coff_header = bsw::load_struct <COFF_HEADER>(is);

		auto optional_header_begin = is.current_pos();

		uint16_t pe_kind;
		is >> pe_kind;

		is.seek(optional_header_begin);

		if (pe_kind == PE_IMAGE_OPTIONAL_HEADER_PE32)
		{
			const OPTIONAL_HEADER_32* header32 = bsw::load_struct<OPTIONAL_HEADER_32>(is);
			assign(m_optional_header, header32);
		}
		else
		{
			if (pe_kind == PE_IMAGE_OPTIONAL_HEADER_PE32_PLUS)
			{
				const OPTIONAL_HEADER_64* header64 = bsw::load_struct<OPTIONAL_HEADER_64>(is);
				assign(m_optional_header, header64);
			}
			else
			{
				reporter.invalid_field_value(abstract_reporter_c::OPTIONAL_HEADER_MAGIC, pe_kind);
				throw std::runtime_error("Unknown type of the OPTIONAL_HEADER");
			}
		}
		m_optional_header.DataDirectory.resize(m_optional_header.NumberOfRvaAndSizes);

		for (u2 i = 0; i < m_optional_header.DataDirectory.size(); i++)
		{
			is >> m_optional_header.DataDirectory[i];
		}
		auto optional_header_end = is.current_pos();
		auto optional_header_len = optional_header_end - optional_header_begin;

		if (optional_header_len != m_coff_header->SizeOfOptionalHeader)
		{
			reporter.invalid_struct_length(abstract_reporter_c::OPTIONAL_HEADER,
				m_coff_header->SizeOfOptionalHeader,
				static_cast<uint32_t>(optional_header_len));
		}
		_check_coff_header(reporter);
	}
	// --------------------------------------------------------------------------------
	void file_c::_check_coff_header(abstract_reporter_c& reporter)
	{
		if (!check_in_enum <COFF_HEADER::PE_IMAGE_FILE_MACHINE>(m_coff_header->Machine))
		{
			reporter.invalid_enum_value(abstract_reporter_c::COFF_HEADER_MACHINE_TYPE, m_coff_header->Machine);
		}
		if (!check_in_flags <COFF_HEADER::PE_IMAGE_FILE_CHARACTERISTICS>(m_coff_header->Characteristics))
		{
			reporter.unknown_flag_set(abstract_reporter_c::COFF_HEADER_CHARACTERISTICS, m_coff_header->Characteristics);
		}
	}
	// --------------------------------------------------------------------------------
	void file_c::_check_optional_header(abstract_reporter_c& reporter)
	{
		if (m_optional_header.ImageBase % 0xFFFF)
		{
			reporter.invalid_field_value(abstract_reporter_c::OPTIONAL_HEADER_IMAGE_BASE, m_optional_header.ImageBase);
		}
		if (!check_in_flags <OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM>(m_optional_header.Subsystem))
		{
			reporter.unknown_flag_set(abstract_reporter_c::OPTIONAL_HEADER_SUBSYSTEM, m_optional_header.Subsystem);
		}
		if (!check_in_flags <OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS>(m_optional_header.DllCharacteristics))
		{
			reporter.unknown_flag_set(abstract_reporter_c::OPTIONAL_HEADER_DLL_CHARACTERISITCS, m_optional_header.DllCharacteristics);
		}
		if (m_optional_header.SectionAlignment < m_optional_header.FileAlignment)
		{
			reporter.invalid_field_value(abstract_reporter_c::OPTIONAL_HEADER_SECTION_ALIGNMENT, m_optional_header.SectionAlignment);
		}
		if (m_optional_header.FileAlignment < 512 || m_optional_header.FileAlignment > 65536 || !isPowerOfTwo(m_optional_header.FileAlignment))
		{
			reporter.invalid_field_value(abstract_reporter_c::OPTIONAL_HEADER_FILE_ALIGNMENT, m_optional_header.FileAlignment);
		}
		if (m_optional_header.Win32VersionValue != 0)
		{
			reporter.should_be_zero(abstract_reporter_c::OPTIONAL_HEADER_WIN32VERSIONVALUE, m_optional_header.Win32VersionValue);
		}
		if (m_optional_header.SizeOfImage % m_optional_header.SectionAlignment)
		{
			reporter.invalid_field_value(abstract_reporter_c::OPTIONAL_HEADER_SIZE_OF_IMAGE, m_optional_header.SizeOfImage);
		}
		if (m_optional_header.LoaderFlags != 0)
		{
			reporter.should_be_zero(abstract_reporter_c::OPTIONAL_HEADER_LOADER_FLAGS, m_optional_header.LoaderFlags);
		}
	}
	// --------------------------------------------------------------------------------
	void file_c::_load_sections(bsw::istream_wrapper_c& stream, abstract_reporter_c& reporter)
	{
		for (u2 i = 0; i < m_coff_header->NumberOfSections; i++)
		{
			SECTION s;
			stream >> s;
			m_sections.push_back(s);
		}
	}
	// --------------------------------------------------------------------------------
	const COFF_HEADER& file_c::coff_header() const
	{
		return *m_coff_header;
	}
	// --------------------------------------------------------------------------------
	const OPTIONAL_HEADER& file_c::optional_header() const
	{
		return m_optional_header;
	}
	// --------------------------------------------------------------------------------
	const file_c::section_t& file_c::sections() const
	{
		return m_sections;
	}
	// ================================================================================
	struct unix_time
	{
		explicit unix_time(uint32_t t)
			: m_time(t)
		{

		}
		std::time_t m_time;
	};
	std::ostream& operator << (std::ostream& os, const unix_time& v)
	{
		std::tm * ptm = std::localtime(&v.m_time);
		char buffer[32];
		std::strftime(buffer, 32, "%d.%m.%Y %H:%M:%S", ptm);
		os << buffer;
		return os;
	}
	// -------------------------------------------------------------------------------------
	struct as_int
	{
		explicit as_int(u1 x)
			: m_v(x)
		{

		}
		u1 m_v;
	};
	std::ostream& operator << (std::ostream& os, const as_int& v)
	{
		union
		{
			u1 b;
			u8 q;
		} u;
		u.q = 0;
		u.b = v.m_v;
		os << u.q;
		return os;
	}
	// -------------------------------------------------------------------------------------
	template <typename T>
	struct flags
	{
		explicit flags(uint32_t f)
			: m_flags(f)
		{

		}
		uint32_t m_flags;
	};
	template <typename T>
	std::ostream& operator << (std::ostream& os, const flags<T>& v)
	{
		auto values = flag_traits<T>::data;
		bool first_time = true;
		for (std::size_t i = 0; i < flag_traits<T>::len(); i++)
		{
			if (v.m_flags & values[i])
			{
				if (!first_time)
				{
					os << ",";
				}
				else
				{
					first_time = false;
				}
				os << static_cast <T>(values[i]);
			}
		}
		return os;
	}
	// -------------------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, COFF_HEADER::PE_IMAGE_FILE_MACHINE v)
	{
		switch (v)
		{
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_UNKNOWN: os << "PE_IMAGE_FILE_MACHINE_UNKNOWN"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_AM33: os << "PE_IMAGE_FILE_MACHINE_AM33"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_AMD64: os << "PE_IMAGE_FILE_MACHINE_AMD64"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_ARM: os << "PE_IMAGE_FILE_MACHINE_ARM"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_ARMNT: os << "PE_IMAGE_FILE_MACHINE_ARMNT"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_EBC: os << "PE_IMAGE_FILE_MACHINE_EBC"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_I386: os << "PE_IMAGE_FILE_MACHINE_I386"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_IA64: os << "PE_IMAGE_FILE_MACHINE_IA64"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_M32R: os << "PE_IMAGE_FILE_MACHINE_M32R"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_MIPS16: os << "PE_IMAGE_FILE_MACHINE_MIPS16"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_MIPSFPU: os << "PE_IMAGE_FILE_MACHINE_MIPSFPU"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_MIPSFPU16: os << "PE_IMAGE_FILE_MACHINE_MIPSFPU16"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_POWERPC: os << "PE_IMAGE_FILE_MACHINE_POWERPC"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_POWERPCFP: os << "PE_IMAGE_FILE_MACHINE_POWERPCFP"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_R4000: os << "PE_IMAGE_FILE_MACHINE_R4000"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_RISCV32: os << "PE_IMAGE_FILE_MACHINE_RISCV32"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_RISCV64: os << "PE_IMAGE_FILE_MACHINE_RISCV64"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_RISCV128: os << "PE_IMAGE_FILE_MACHINE_RISCV128"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_SH3: os << "PE_IMAGE_FILE_MACHINE_SH3"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_SH3DSP: os << "PE_IMAGE_FILE_MACHINE_SH3DSP"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_SH4: os << "PE_IMAGE_FILE_MACHINE_SH4"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_SH5: os << "PE_IMAGE_FILE_MACHINE_SH5"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_THUMB: os << "PE_IMAGE_FILE_MACHINE_THUMB"; break;
		case COFF_HEADER::PE_IMAGE_FILE_MACHINE_WCEMIPSV2: os << "PE_IMAGE_FILE_MACHINE_WCEMIPSV2"; break;
		default:
			os << "--- UNKNOWN ---";
		}
		return os;
	}
	// -----------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, COFF_HEADER::PE_IMAGE_FILE_CHARACTERISTICS v)
	{
		switch (v)
		{
		case COFF_HEADER::PE_IMAGE_FILE_RELOCS_STRIPPED: os << "PE_IMAGE_FILE_RELOCS_STRIPPED"; break;
		case COFF_HEADER::PE_IMAGE_FILE_EXECUTABLE_IMAGE: os << "PE_IMAGE_FILE_EXECUTABLE_IMAGE"; break;
		case COFF_HEADER::PE_IMAGE_FILE_LINE_NUMS_STRIPPED: os << "PE_IMAGE_FILE_LINE_NUMS_STRIPPED"; break;
		case COFF_HEADER::PE_IMAGE_FILE_LOCAL_SYMS_STRIPPED: os << "PE_IMAGE_FILE_LOCAL_SYMS_STRIPPED"; break;
		case COFF_HEADER::PE_IMAGE_FILE_AGGRESSIVE_WS_TRIM: os << "PE_IMAGE_FILE_AGGRESSIVE_WS_TRIM"; break;
		case COFF_HEADER::PE_IMAGE_FILE_LARGE_ADDRESS_AWARE: os << "PE_IMAGE_FILE_LARGE_ADDRESS_AWARE"; break;
		case COFF_HEADER::PE_IMAGE_FILE_RESERVED: os << "PE_IMAGE_FILE_RESERVED"; break;
		case COFF_HEADER::PE_IMAGE_FILE_BYTES_REVERSED_LO: os << "PE_IMAGE_FILE_BYTES_REVERSED_LO"; break;
		case COFF_HEADER::PE_IMAGE_FILE_32BIT_MACHINE: os << "PE_IMAGE_FILE_32BIT_MACHINE"; break;
		case COFF_HEADER::PE_IMAGE_FILE_DEBUG_STRIPPED: os << "PE_IMAGE_FILE_DEBUG_STRIPPED"; break;
		case COFF_HEADER::PE_IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP: os << "PE_IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP"; break;
		case COFF_HEADER::PE_IMAGE_FILE_NET_RUN_FROM_SWAP: os << "PE_IMAGE_FILE_NET_RUN_FROM_SWAP"; break;
		case COFF_HEADER::PE_IMAGE_FILE_SYSTEM: os << "PE_IMAGE_FILE_SYSTEM"; break;
		case COFF_HEADER::PE_IMAGE_FILE_DLL: os << "PE_IMAGE_FILE_DLL"; break;
		case COFF_HEADER::PE_IMAGE_FILE_UP_SYSTEM_ONLY: os << "PE_IMAGE_FILE_UP_SYSTEM_ONLY"; break;
		case COFF_HEADER::PE_IMAGE_FILE_BYTES_REVERSED_HI: os << "PE_IMAGE_FILE_BYTES_REVERSED_HI"; break;
		default:
			os << "--- UNKNOWN ---";
		}
		return os;
	}
	// -------------------------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, const COFF_HEADER& v)
	{
		os << "Machine              = " << static_cast<COFF_HEADER::PE_IMAGE_FILE_MACHINE>(v.Machine) << std::endl;
		os << "NumberOfSections     = " << v.NumberOfSections << std::endl;
		os << "TimeDateStamp        = " << unix_time(v.TimeDateStamp) << std::endl;
		os << "PointerToSymbolTable = " << v.PointerToSymbolTable << std::endl;
		os << "NumberOfSymbols      = " << v.NumberOfSymbols << std::endl;
		os << "SizeOfOptionalHeader = " << v.SizeOfOptionalHeader << std::endl;
		os << "Characteristics      = " << flags<COFF_HEADER::PE_IMAGE_FILE_CHARACTERISTICS>(v.Characteristics) << std::endl;
		return os;
	}
	// -------------------------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM v)
	{
		switch (v)
		{
		case OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_UNKNOWN: os << "PE_IMAGE_SUBSYSTEM_UNKNOWN"; break;
		case OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_NATIVE: os << "PE_IMAGE_SUBSYSTEM_NATIVE"; break;
		case OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_WINDOWS_GUI: os << "PE_IMAGE_SUBSYSTEM_WINDOWS_GUI"; break;
		case OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_WINDOWS_CUI: os << "PE_IMAGE_SUBSYSTEM_WINDOWS_CUI"; break;
		case OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_POSIX_CUI: os << "PE_IMAGE_SUBSYSTEM_POSIX_CUI"; break;
		case OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_WINDOWS_CE_GUI: os << "PE_IMAGE_SUBSYSTEM_WINDOWS_CE_GUI"; break;
		case OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_EFI_APPLICATION: os << "PE_IMAGE_SUBSYSTEM_EFI_APPLICATION"; break;
		case OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER: os << "PE_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER"; break;
		case OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER: os << "PE_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER"; break;
		case OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM_EFI_ROM: os << "PE_IMAGE_SUBSYSTEM_EFI_ROM"; break;
		default: os << "--- UNKNOWN ---";
		}
		return os;
	}
	// ----------------------------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS v)
	{
		switch (v)
		{
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_RESERVED1: os << "PE_IMAGE_DLLCHARACTERISTICS_RESERVED1"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_RESERVED2: os << "PE_IMAGE_DLLCHARACTERISTICS_RESERVED2"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_RESERVED3: os << "PE_IMAGE_DLLCHARACTERISTICS_RESERVED3"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_RESERVED4: os << "PE_IMAGE_DLLCHARACTERISTICS_RESERVED4"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA: os << "PE_IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE: os << "PE_IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY: os << "PE_IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_NX_COMPAT: os << "PE_IMAGE_DLLCHARACTERISTICS_NX_COMPAT"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_NO_ISOLATION: os << "PE_IMAGE_DLLCHARACTERISTICS_NO_ISOLATION"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_NO_SEH: os << "PE_IMAGE_DLLCHARACTERISTICS_NO_SEH"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_NO_BIND: os << "PE_IMAGE_DLLCHARACTERISTICS_NO_BIND"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_APPCONTAINER: os << "PE_IMAGE_DLLCHARACTERISTICS_APPCONTAINER"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_WDM_DRIVER: os << "PE_IMAGE_DLLCHARACTERISTICS_WDM_DRIVER"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_GUARD_CF: os << "PE_IMAGE_DLLCHARACTERISTICS_GUARD_CF"; break;
		case OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE: os << "PE_IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE"; break;
		default: os << "--- UNKNOWN ---";
		}
		return os;
	}
	// -------------------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, const OPTIONAL_HEADER& v)
	{
		os << "Is64Bit =                     " << v.Is64Bit << std::endl;
		os << "MajorLinkerVersion =          " << as_int(v.MajorLinkerVersion) << std::endl;
		os << "MinorLinkerVersion =          " << as_int(v.MinorLinkerVersion) << std::endl;
		os << "SizeOfCode =                  " << v.SizeOfCode << std::endl;
		os << "SizeOfInitializedData =       " << v.SizeOfInitializedData << std::endl;
		os << "SizeOfUninitializedData =     " << v.SizeOfUninitializedData << std::endl;
		os << "AddressOfEntryPoint =         " << v.AddressOfEntryPoint << std::endl;
		os << "BaseOfCode =                  " << v.BaseOfCode << std::endl;
		if (!v.Is64Bit)
		{
			os << "BaseOfData =                  " << v.BaseOfData << std::endl;
		}
		os << "ImageBase =                   " << v.ImageBase << std::endl;
		os << "SectionAlignment =            " << v.SectionAlignment << std::endl;
		os << "FileAlignment =               " << v.FileAlignment << std::endl;
		os << "MajorOperatingSystemVersion = " << v.MajorOperatingSystemVersion << std::endl;
		os << "MinorOperatingSystemVersion = " << v.MinorOperatingSystemVersion << std::endl;
		os << "MajorImageVersion =           " << v.MajorImageVersion << std::endl;
		os << "MinorImageVersion =           " << v.MinorImageVersion << std::endl;
		os << "MajorSubsystemVersion =       " << v.MajorSubsystemVersion << std::endl;
		os << "MinorSubsystemVersion =       " << v.MinorSubsystemVersion << std::endl;
		os << "Win32VersionValue =           " << v.Win32VersionValue << std::endl;
		os << "SizeOfImage =                 " << v.SizeOfImage << std::endl;
		os << "SizeOfHeaders =               " << v.SizeOfHeaders << std::endl;
		os << "CheckSum =                    " << v.CheckSum << std::endl;
		os << "Subsystem =                   " << static_cast <OPTIONAL_HEADER::PE_IMAGE_SUBSYSTEM>(v.Subsystem) << std::endl;
		os << "DllCharacteristics =          " << flags <OPTIONAL_HEADER::PE_IMAGE_DLLCHARACTERISTICS>(v.DllCharacteristics) << std::endl;
		os << "SizeOfStackReserve =          " << v.SizeOfStackReserve << std::endl;
		os << "SizeOfStackCommit =           " << v.SizeOfStackCommit << std::endl;
		os << "SizeOfHeapReserve =           " << v.SizeOfHeapReserve << std::endl;
		os << "SizeOfHeapCommit =            " << v.SizeOfHeapCommit << std::endl;
		os << "LoaderFlags =                 " << v.LoaderFlags << std::endl;
		os << "NumberOfRvaAndSizes =         " << v.NumberOfRvaAndSizes << std::endl;
		return os;
	}
	// ================================================================================
	SECTION_NAME::SECTION_NAME()
		: name{ 0 }
	{

	}
	// --------------------------------------------------------------------------------
	SECTION_NAME::SECTION_NAME(u8 a)
		: name{ a }
	{

	}
	// --------------------------------------------------------------------------------
	std::string SECTION_NAME::to_string() const
	{
		if (name.Str[7] == 0)
		{
			return name.Str;
		}
		return std::string(name.Str, name.Str + 7);
	}
	// --------------------------------------------------------------------------------
	u8 SECTION_NAME::qword() const
	{
		return name.Qword;
	}
	// --------------------------------------------------------------------------------
	bool operator < (const SECTION_NAME& a, const SECTION_NAME& b)
	{
		return a.qword() < b.qword();
	}
	// --------------------------------------------------------------------------------
	bool operator == (const SECTION_NAME& a, const SECTION_NAME& b)
	{
		return a.qword() == b.qword();
	}
	// --------------------------------------------------------------------------------
	bool operator != (const SECTION_NAME& a, const SECTION_NAME& b)
	{
		return !(a == b);
	}
	// --------------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, const SECTION_NAME& s)
	{
		os << s.to_string();
		return os;
	}
	// --------------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, SECTION::CHARACTERISITICS v)
	{
		switch (v)
		{
		case SECTION::PE_IMAGE_SCN_TYPE_NO_PAD:            os << "PE_IMAGE_SCN_TYPE_NO_PAD"; break;
		case SECTION::PE_IMAGE_SCN_CNT_CODE:               os << "PE_IMAGE_SCN_CNT_CODE"; break;
		case SECTION::PE_IMAGE_SCN_CNT_INITIALIZED_DATA:   os << "PE_IMAGE_SCN_CNT_INITIALIZED_DATA"; break;
		case SECTION::PE_IMAGE_SCN_CNT_UNINITIALIZED_DATA: os << "PE_IMAGE_SCN_CNT_UNINITIALIZED_DATA"; break;
		case SECTION::PE_IMAGE_SCN_LNK_OTHER:              os << "PE_IMAGE_SCN_LNK_OTHER"; break;
		case SECTION::PE_IMAGE_SCN_LNK_INFO:               os << "PE_IMAGE_SCN_LNK_INFO"; break;
		case SECTION::PE_IMAGE_SCN_LNK_REMOVE:             os << "PE_IMAGE_SCN_LNK_REMOVE"; break;
		case SECTION::PE_IMAGE_SCN_LNK_COMDAT:             os << "PE_IMAGE_SCN_LNK_COMDAT"; break;
		case SECTION::PE_IMAGE_SCN_GPREL:                  os << "PE_IMAGE_SCN_GPREL"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_1BYTES:           os << "PE_IMAGE_SCN_ALIGN_1BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_2BYTES:           os << "PE_IMAGE_SCN_ALIGN_2BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_4BYTES:           os << "PE_IMAGE_SCN_ALIGN_4BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_8BYTES:           os << "PE_IMAGE_SCN_ALIGN_8BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_16BYTES:          os << "PE_IMAGE_SCN_ALIGN_16BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_32BYTES:          os << "PE_IMAGE_SCN_ALIGN_32BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_64BYTES:          os << "PE_IMAGE_SCN_ALIGN_64BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_128BYTES:         os << "PE_IMAGE_SCN_ALIGN_128BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_256BYTES:         os << "PE_IMAGE_SCN_ALIGN_256BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_512BYTES:         os << "PE_IMAGE_SCN_ALIGN_512BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_1024BYTES:        os << "PE_IMAGE_SCN_ALIGN_1024BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_2048BYTES:        os << "PE_IMAGE_SCN_ALIGN_2048BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_4096BYTES:        os << "PE_IMAGE_SCN_ALIGN_4096BYTES"; break;
		case SECTION::PE_IMAGE_SCN_ALIGN_8192BYTES:        os << "PE_IMAGE_SCN_ALIGN_8192BYTES"; break;
		case SECTION::PE_IMAGE_SCN_LNK_NRELOC_OVFL:        os << "PE_IMAGE_SCN_LNK_NRELOC_OVFL"; break;
		case SECTION::PE_IMAGE_SCN_MEM_DISCARDABLE:        os << "PE_IMAGE_SCN_MEM_DISCARDABLE"; break;
		case SECTION::PE_IMAGE_SCN_MEM_NOT_CACHED:         os << "PE_IMAGE_SCN_MEM_NOT_CACHED"; break;
		case SECTION::PE_IMAGE_SCN_MEM_NOT_PAGED:          os << "PE_IMAGE_SCN_MEM_NOT_PAGED"; break;
		case SECTION::PE_IMAGE_SCN_MEM_SHARED:             os << "PE_IMAGE_SCN_MEM_SHARED"; break;
		case SECTION::PE_IMAGE_SCN_MEM_EXECUTE:            os << "PE_IMAGE_SCN_MEM_EXECUTE"; break;
		case SECTION::PE_IMAGE_SCN_MEM_READ:               os << "PE_IMAGE_SCN_MEM_READ"; break;
		case SECTION::PE_IMAGE_SCN_MEM_WRITE:              os << "PE_IMAGE_SCN_MEM_WRITE"; break;
		}
		return os;
	}
	// ---------------------------------------------------------------------------------
	std::ostream& operator << (std::ostream& os, const SECTION& v)
	{
		os << "Name                 = " << v.Name << std::endl;
		os << "VirtualSize          = " << v.VirtualSize << std::endl;
		os << "VirtualAddress       = " << v.VirtualAddress << std::endl;
		os << "SizeOfRawData        = " << v.SizeOfRawData << std::endl;
		os << "PointerToRawData     = " << v.PointerToRawData << std::endl;
		os << "PointerToRelocations = " << v.PointerToRelocations << std::endl;
		os << "PointerToLinenumbers = " << v.PointerToLinenumbers << std::endl;
		os << "NumberOfRelocations  = " << v.NumberOfRelocations << std::endl;
		os << "NumberOfLinenumbers  = " << v.NumberOfLinenumbers << std::endl;
		os << "Characteristics      = " << flags <SECTION::CHARACTERISITICS>(v.Characteristics) << std::endl;

		return os;
	}
} // ns pefile


#undef MIO_BASIC_MMAP_IMPL
#include "detail/mmap.ipp"
