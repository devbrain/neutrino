#include "clr_res.hpp"
#include <string.h>

namespace pefile
{
	clr_resource_c::clr_resource_c()
	{
	}

	clr_resource_c::~clr_resource_c()
	{
	}
	using DWORD = uint32_t;
	using INT = int;
	using UINT = unsigned int;
	using ULONG_PTR = uintptr_t;
#define FALSE false
#define TRUE true
#define RESOURCES_MAGIC_NUMBER			0xBEEFCACE
	bool clr_resource_c::ProcessResourcesFile(const uint8_t* pAddress, uint32_t uSize)
	{
		const uint8_t* ptr = pAddress;

		//
		// Collect basic information: pointer and size of the file
		//

		pBaseAddress = ptr;
		Size = uSize;

		//
		// Read the magic number, its value has to be: 0xBEEFCACE
		//

		DWORD MagicNumber;

		MagicNumber = *(DWORD *)ptr;
		ptr += sizeof(DWORD);

		if (MagicNumber != RESOURCES_MAGIC_NUMBER)
			return FALSE;

		[[maybe_unused]] DWORD NumberOfReaderTypes;

		NumberOfReaderTypes = *(DWORD *)ptr;
		ptr += sizeof(DWORD);

		DWORD SizeOfReaderTypes;

		SizeOfReaderTypes = *(DWORD *)ptr;
		ptr += sizeof(DWORD);

		//
		// Skip ReaderTypes
		//

		ptr += SizeOfReaderTypes;

		//

		Version = *(DWORD *)ptr;
		ptr += sizeof(DWORD);

		//
		// Read number of resources
		//

		NumberOfResources = *(DWORD *)ptr;
		ptr += sizeof(DWORD);

		//
		// Read number of types
		//

		NumberOfTypes = *(DWORD *)ptr;
		ptr += sizeof(DWORD);

		//
		// Skip Types: (CHAR *Type;) * NumOfTypes
		// (Save position)
		//

		pTypes = ptr;

		for (UINT x = 0; x < NumberOfTypes; x++)
		{
			INT StringSize = 0;
			UINT ValueSize = 0;

			if (!DecodeInt(ptr, &StringSize, &ValueSize))
				return FALSE;
			ptr += ValueSize;

			ptr += StringSize;
		}

		//
		// Alignes position
		//

		DWORD Position = (DWORD)(((ULONG_PTR)ptr) - ((ULONG_PTR)pBaseAddress));

		DWORD Aligned = Position & 7;

		if (Aligned != 0)
		{
			ptr += (8 - Aligned);
		}

		//
		// Skip name hashes
		//

		ptr += (sizeof(DWORD) * NumberOfResources);

		//
		// Skip name positions (first save location)
		//

		pNamesOffsets = ptr;

		ptr += (sizeof(DWORD) * NumberOfResources);

		//
		// Read Data Section Offset
		//

		DWORD DataSectionOffset;

		DataSectionOffset = *(DWORD *)ptr;
		ptr += sizeof(DWORD);

		pDataSection = (const uint8_t*)(DataSectionOffset + ((ULONG_PTR)pBaseAddress));

		//
		// Save names position
		//

		pNames = ptr;

		return TRUE;
	}

	//
	// Decode ints who have been encoded in 7 bits
	//

	bool clr_resource_c::DecodeInt(const uint8_t*  pAddress, int *Value, unsigned int *uSize)
	{
		const uint8_t *ptr = pAddress;

		uint8_t c;
		INT a = 0, b = 0;
		UINT x = 0;

		while (TRUE)
		{
			c = *ptr;
			ptr++;

			a |= ((c & 0x7F) << (b & 0x1F));
			b += 7;

			x++;

			if ((c & 0x80) == 0) break;
		}

		if (Value) *Value = a;
		if (uSize) *uSize = x;

		return TRUE;
	}

	//
	// Read the name from a given resource
	//

	bool clr_resource_c::ReadName(const uint32_t nResource, wchar_t *Str, uint32_t Len)
	{
		DWORD NameOffset = *(DWORD *)((nResource * sizeof(DWORD)) +
			((ULONG_PTR)pNamesOffsets));

		if (NameOffset > (DWORD)(((ULONG_PTR)pNames) - ((ULONG_PTR)pDataSection)))
			return FALSE;

		memset(Str, 0, Len * sizeof(wchar_t));

		const uint8_t *ptr = (const uint8_t *)(NameOffset + ((ULONG_PTR)pNames));

		INT NameSize = 0;
		UINT ValueSize = 0;

		if (!DecodeInt(ptr, &NameSize, &ValueSize))
			return FALSE;
		ptr += ValueSize;

		memcpy(Str, ptr, NameSize);

		return TRUE;
	}

	//
	// Collect Resource Info
	//

	bool clr_resource_c::GetResourceInfo(uint32_t nResource, wchar_t *Str, uint32_t Len, uint32_t *Offset, int *TypeIndex)
	{
		//
		// Read name
		//

		DWORD NameOffset = *(DWORD *)((nResource * sizeof(DWORD)) +
			((ULONG_PTR)pNamesOffsets));

		if (NameOffset > (DWORD)(((ULONG_PTR)pNames) - ((ULONG_PTR)pDataSection)))
			return FALSE;

		memset(Str, 0, Len * sizeof(wchar_t));

		const uint8_t *ptr = (const uint8_t *)(NameOffset + ((ULONG_PTR)pNames));

		INT NameSize = 0;
		UINT ValueSize = 0;

		if (!DecodeInt(ptr, &NameSize, &ValueSize))
			return FALSE;
		ptr += ValueSize;

		memcpy(Str, ptr, NameSize);

		ptr += NameSize;

		//
		// After reading the name
		//

		DWORD DataOffset = *(DWORD *)ptr;

		const uint8_t *pData = (const uint8_t *)(DataOffset + ((ULONG_PTR)pDataSection));

		//
		// Collect info
		//

		if (Offset)
		{
			*Offset = (DWORD)(((ULONG_PTR)pData) - ((ULONG_PTR)pBaseAddress));
		}

		if (TypeIndex)
		{
			*TypeIndex = 0;
			ValueSize = 0;

			if (!DecodeInt(pData, TypeIndex, &ValueSize)) 	return FALSE;
		}

		return TRUE;
	}
}