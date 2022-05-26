#ifndef __PEFILE_CLR_RES_HPP__
#define __PEFILE_CLR_RES_HPP__

#include <stdint.h>

namespace pefile
{
	class clr_resource_c
	{

	public:

		clr_resource_c();
		~clr_resource_c();

		const uint8_t* pBaseAddress;
		uint32_t Size;

		uint32_t Version;
		uint32_t NumberOfResources;
		uint32_t NumberOfTypes;

		const uint8_t* pTypes;
		const uint8_t* pNamesOffsets;
		const uint8_t* pDataSection;
		const uint8_t* pNames;

		bool ProcessResourcesFile(const uint8_t* pAddress, uint32_t uSize);
		bool ReadName(const uint32_t nResource, wchar_t *Str, uint32_t Len);
		bool GetResourceInfo(uint32_t nResource, wchar_t *Str, uint32_t Len, uint32_t *Offset, int *TypeIndex);

	private:
		bool DecodeInt(const uint8_t*  pAddress, int *Value, unsigned int *uSize);
	};

} // ns pefile

#endif
