#include <sstream>
#if defined(_WIN32)
#include <Windows.h>
#include <AccCtrl.h>
#include <Aclapi.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif

#include "shm.hpp"

#include "error.hpp"
#include "wchar.hpp"

namespace
{
#if defined(_WIN32)
	class auto_sid
	{
	public:
		auto_sid(PSID psid)
			: m_psid(psid)
		{

		}
		~auto_sid()
		{
			if (m_psid)
			{
				FreeSid(m_psid);
			}
		}
		void release()
		{
			m_psid = 0;
		}
	private:
		PSID m_psid;
	};

	template <typename T>
	class auto_local_free
	{
	public:
		auto_local_free(T ptr)
			: m_ptr(ptr)
		{

		}
		~auto_local_free()
		{
			if (m_ptr)
			{
				LocalFree(m_ptr);
			}
		}
		void release()
		{
			m_ptr = 0;
		}
	private:
		T m_ptr;
	};

	static HANDLE create_shmem(const char* seg_name, DWORD size, void** pp, bool is_global)
	{
		PSID pEveryoneSID = NULL;
		SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
		// Create a well-known SID for the Everyone group.
		if (!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pEveryoneSID))
		{
			RAISE_SYSTEM_ERROR_WITH_CODE("AllocateAndInitializeSid", GetLastError());
		}
		auto_sid sid_holder(pEveryoneSID);
		// Initialize an EXPLICIT_ACCESS structure for an ACE.
		// The ACE will allow Everyone all access to the shared memory object.

		EXPLICIT_ACCESS ea;
		ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
		ea.grfAccessPermissions = FILE_MAP_ALL_ACCESS;
		ea.grfAccessMode = SET_ACCESS;
		ea.grfInheritance = NO_INHERITANCE;
		ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		ea.Trustee.ptstrName = (LPTSTR)pEveryoneSID;

		// Create a new ACL that contains the new ACEs.
		PACL pACL = NULL;
		DWORD dwRes = SetEntriesInAcl(1, &ea, NULL, &pACL);
		if (ERROR_SUCCESS != dwRes)
		{
			RAISE_SYSTEM_ERROR_WITH_CODE("SetEntriesInAcl", GetLastError());
		}
		auto_local_free <PACL> pacl_holder(pACL);
		// Initialize a security descriptor.  
		PSECURITY_DESCRIPTOR pSD = NULL;
		pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
		if (NULL == pSD)
		{
			RAISE_SYSTEM_ERROR_WITH_CODE("LocalAlloc", GetLastError());
		}

		auto_local_free <PSECURITY_DESCRIPTOR> psd_holder(pSD);

		if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
		{
			RAISE_SYSTEM_ERROR_WITH_CODE("InitializeSecurityDescriptor", GetLastError());
		}

		// Add the ACL to the security descriptor. 
		if (!SetSecurityDescriptorDacl(pSD,
			TRUE,     // bDaclPresent flag   
			pACL,
			FALSE) // not a default DACL 
			)
		{
			RAISE_SYSTEM_ERROR_WITH_CODE("SetSecurityDescriptorDacl", GetLastError());
		}
		SECURITY_ATTRIBUTES sa;
		sa.nLength = sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = pSD;
		sa.bInheritHandle = FALSE;
		HANDLE hMap = NULL;
		if (is_global)
		{
			std::ostringstream os;
			os << "Global\\" << seg_name;
			hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, size, os.str().c_str());
			if (!hMap)
			{
				RAISE_SYSTEM_ERROR_WITH_CODE("CreateFileMappingA", GetLastError());
			}
		}
		else
		{
			hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0, size, seg_name);
			RAISE_SYSTEM_ERROR_WITH_CODE("CreateFileMappingA", GetLastError());
		}


		if (hMap)
		{
			if (GetLastError() == ERROR_ALREADY_EXISTS)
			{
				CloseHandle(hMap);
				hMap = NULL;
			}
			else
			{
				*pp = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			}
		}
		return hMap;
	}
	// =============================================================================================
	HANDLE attach_shmem(const char* seg_name, void** pp, bool is_global)
	{
		HANDLE hMap;

		if (is_global)
		{
			std::ostringstream os;
			os << "Global\\" << seg_name;
			hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, os.str().c_str());
		}
		else
		{
			hMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, seg_name);
		}

		if (!hMap)
		{
			RAISE_SYSTEM_ERROR_WITH_CODE("OpenFileMappingA", GetLastError());
		}

		if (pp)
		{
			*pp = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		}

		return hMap;
	}
	// ---------------------------------------------------------------------------------
	void detach_shmem(HANDLE hMap, void* p)
	{
		if (p)
		{
			UnmapViewOfFile(p);
		}
		CloseHandle(hMap);
	}
#endif
} // anonymous ns

namespace bsw
{
#if _WIN32
	shared_memory_c::~shared_memory_c()
	{
		if (handle)
		{
			detach_shmem(handle, mem);
		}
	}
	// --------------------------------------------------------------------------------------------------
	void* shared_memory_c::ptr ()
	{
		return mem;
	}
	// --------------------------------------------------------------------------------------------------
	const void* shared_memory_c::ptr () const
	{
		return mem;
	}
	// --------------------------------------------------------------------------------------------------
	shared_memory_c::shared_memory_c(const char* name, unsigned int size, bool is_global)
		: handle(0),
		mem(0)
	{
		handle = create_shmem(name, size, &mem, is_global);
	}
	// --------------------------------------------------------------------------------------------------
	shared_memory_c::shared_memory_c(const char* name, bool is_global)
		: handle(0),
		mem(0)
	{
		handle = attach_shmem(name, &mem, is_global);
	}
	// --------------------------------------------------------------------------------------------------
	bool shared_memory_c::can_create_global(void* token)
	{
		BOOL ok, has_priv;
		LUID priv_id;
		PRIVILEGE_SET privs;
		HANDLE hToken = static_cast<HANDLE>(token);
	
		ok = LookupPrivilegeValue(NULL, SE_CREATE_GLOBAL_NAME, &priv_id);

		if (ok)
		{
			privs.PrivilegeCount = 1;
			privs.Control = PRIVILEGE_SET_ALL_NECESSARY;
			privs.Privilege[0].Luid = priv_id;
			privs.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
			ok = PrivilegeCheck(hToken, &privs, &has_priv);
			if (has_priv)
			{
				return true;
			}
		}
		return false;
	}
	// ---------------------------------------------------------------------------
	bool shared_memory_c::can_create_global()
	{
		HANDLE hToken;

		BOOL ok = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
		if (!ok && GetLastError() == ERROR_NO_TOKEN) 
		{
			/* no thread-specific access token, so try to get process access token
			*/
			ok = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
		}

		if (ok)
		{
			return can_create_global(hToken);
		}
		return false;
	}
#endif
	// ======================================================================
	namespace memory_mapped_file
	{
		unsigned int mmf_granularity()
		{
#if defined(_WIN32)
			SYSTEM_INFO SystemInfo;
			GetSystemInfo(&SystemInfo);
			return SystemInfo.dwAllocationGranularity;
#else
			return sysconf(_SC_PAGE_SIZE);
#endif
		}

		base_mmf::base_mmf() :
			data_(0),
			offset_(0),
			mapped_size_(0),
			file_size_(0),
			granularity_(mmf_granularity()),
#if defined(_WIN32)
			file_handle_(INVALID_HANDLE_VALUE),
			file_mapping_handle_(INVALID_HANDLE_VALUE)
#else
			file_handle_(-1)
#endif
		{
		}

		base_mmf::~base_mmf()
		{
			close();
		}

		void base_mmf::close()
		{
			unmap();
#if defined(_WIN32)
			::CloseHandle(file_handle_);
			file_handle_ = (void*)-1;
#else
			::close(file_handle_);
			file_handle_ = -1;
#endif
			file_size_ = 0;
		}

		void base_mmf::unmap()
		{
			if (data_)
			{
				char* real_data = data_
					- (offset_ - offset_ / granularity_ * granularity_);
#if defined(_WIN32)
				::UnmapViewOfFile(real_data);
				
#else
				size_t real_mapped_size = mapped_size_ + (data_ - real_data);
				::munmap(const_cast<char*>(real_data), real_mapped_size);
#endif
			}
#if defined(_WIN32)
			if (file_mapping_handle_ != INVALID_HANDLE_VALUE)
			{
				::CloseHandle(file_mapping_handle_);
				file_mapping_handle_ = INVALID_HANDLE_VALUE;
			}
#endif
			data_ = 0;
			offset_ = 0;
			mapped_size_ = 0;
		}

		uint64_t base_mmf::query_file_size_()
		{
#if defined(_WIN32)
			LARGE_INTEGER ul;
			DWORD high_size;
			ul.LowPart = GetFileSize(file_handle_, &high_size);
			ul.HighPart = high_size;
			return ul.QuadPart;
#else
			struct stat sbuf;
			if (::fstat(file_handle_, &sbuf) == -1) return 0;
			return sbuf.st_size;
#endif
		}

		read_only_mmf::read_only_mmf(char const* pathname, bool map_all)
		{
			open(pathname, map_all);
		}

		read_only_mmf::read_only_mmf(wchar_t const* pathname, bool map_all)
		{
			open(pathname, map_all);
		}

		void read_only_mmf::open(char const* pathname, bool map_all)
		{
			if (!pathname) return;
			if (is_open()) close();
#if defined(_WIN32)
			file_handle_ = ::CreateFile(pathname, GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (file_handle_ == INVALID_HANDLE_VALUE) return;
#else
			file_handle_ = ::open(pathname, O_RDONLY);
			if (file_handle_ == -1) return;
#endif
			file_size_ = query_file_size_();
			if (map_all) map();
		}

		void read_only_mmf::open(wchar_t const* pathname, bool map_all)
		{
			if (!pathname) return;
			if (is_open()) close();
#if defined(_WIN32)
			file_handle_ = ::CreateFileW(pathname, GENERIC_READ,
				FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
				OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (file_handle_ == INVALID_HANDLE_VALUE) return;
#else
            auto p = wstring_to_utf8(pathname);
			file_handle_ = ::open(p.c_str(), O_RDONLY);
			if (file_handle_ == -1) return;
#endif
			file_size_ = query_file_size_();
			if (map_all) map();
		}
#if !defined(_WIN32)
    typedef uint8_t BYTE;
    typedef uint32_t DWORD;
    typedef int32_t LONG;
    typedef int64_t LONGLONG;

    typedef union _LARGE_INTEGER {
      struct {
        DWORD LowPart;
        LONG  HighPart;
      };
      struct {
        DWORD LowPart;
        LONG  HighPart;
      } u;
      LONGLONG QuadPart;
    } LARGE_INTEGER, *PLARGE_INTEGER;
#endif
		void read_only_mmf::map(
			size_t offset, size_t requested_size)
		{
			unmap();
			if (offset >= file_size_) return;
			auto mapping_size = requested_size && offset + requested_size
				< file_size_ ? requested_size : file_size_ - offset;
			if (mapping_size <= 0) return;
			LARGE_INTEGER real_offset;
			real_offset.QuadPart = offset / granularity_ * granularity_;
#if defined(_WIN32)
			file_mapping_handle_ = ::CreateFileMapping(
				file_handle_, 0, PAGE_READONLY, (offset + mapping_size) >> 32,
				(offset + mapping_size) & 0xFFFFFFFF, 0);
			if (file_mapping_handle_ == INVALID_HANDLE_VALUE) return;
			char* real_data = static_cast<char*>(::MapViewOfFile(
				file_mapping_handle_, FILE_MAP_READ, real_offset.HighPart,
				real_offset.LowPart, offset - real_offset.QuadPart + mapping_size));
			if (!real_data) return;
#else
			char* real_data = static_cast<char*>(::mmap(
				0, offset - real_offset.QuadPart + mapping_size, PROT_READ, MAP_SHARED,
				file_handle_, real_offset.QuadPart));
			if (real_data == MAP_FAILED) return;
#endif
			data_ = real_data + (offset - real_offset.QuadPart);
			mapped_size_ = mapping_size;
			offset_ = offset;
		}

		writable_mmf::writable_mmf(char const* pathname,
			memory_mapped_file::mmf_exists_mode exists_mode,
			memory_mapped_file::mmf_doesnt_exist_mode doesnt_exist_mode)
		{
			open(pathname, exists_mode, doesnt_exist_mode);
		}

		void writable_mmf::open(char const* pathname,
			memory_mapped_file::mmf_exists_mode exists_mode,
			memory_mapped_file::mmf_doesnt_exist_mode doesnt_exist_mode)
		{
			if (!pathname) return;
			if (is_open()) close();
#if defined(_WIN32)
			int win_open_mode;

			switch (exists_mode)
			{
			case if_exists_just_open:
			case if_exists_map_all:
				win_open_mode = doesnt_exist_mode == if_doesnt_exist_create ?
					OPEN_ALWAYS : OPEN_EXISTING;
				break;
			case if_exists_truncate:
				win_open_mode = doesnt_exist_mode == if_doesnt_exist_create ?
					CREATE_ALWAYS : TRUNCATE_EXISTING;
				break;
			default:
				if (doesnt_exist_mode == if_doesnt_exist_create)
				{
					win_open_mode = CREATE_NEW;
				}
				else return;
			}

			file_handle_ = ::CreateFile(pathname, GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
				win_open_mode, FILE_ATTRIBUTE_NORMAL, 0);
			if (file_handle_ == INVALID_HANDLE_VALUE) return;
#else
			int posix_open_mode = O_RDWR;
			switch (exists_mode)
			{
			case if_exists_just_open:
			case if_exists_map_all:
				posix_open_mode |= doesnt_exist_mode == if_doesnt_exist_create ?
					O_CREAT : 0;
				break;
			case if_exists_truncate:
				posix_open_mode |= doesnt_exist_mode == if_doesnt_exist_create ?
					O_TRUNC | O_CREAT : O_TRUNC;
				break;
			default:
				if (doesnt_exist_mode == if_doesnt_exist_create)
					posix_open_mode |= O_EXCL | O_CREAT;
				else return;
			}
			file_handle_ = ::open(pathname, posix_open_mode,
				S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (file_handle_ == -1) return;
#endif
			file_size_ = query_file_size_();
			if (exists_mode == if_exists_map_all && file_size_ > 0) map();
		}

		void writable_mmf::map(size_t offset, size_t requested_size)
		{
			unmap();
			if (offset > file_size_) return;
			size_t mapping_size = requested_size ?
				requested_size : file_size_ - offset;
			LARGE_INTEGER real_offset;
			real_offset.QuadPart = offset / granularity_ * granularity_;
#if defined(_WIN32)
			LARGE_INTEGER max_size;
			max_size.QuadPart = offset + mapping_size;
			file_mapping_handle_ = ::CreateFileMappingA(
				file_handle_, 0, PAGE_READWRITE, max_size.HighPart,
				max_size.LowPart, 0);
			if (file_mapping_handle_ == INVALID_HANDLE_VALUE) return;
			char* real_data = static_cast<char*>(::MapViewOfFile(
				file_mapping_handle_, FILE_MAP_WRITE, real_offset.HighPart,
				real_offset.LowPart, offset - real_offset.QuadPart + mapping_size));
			if (!real_data) return;
#else
			if (offset + mapping_size > file_size_)
			{
				if (-1 == ftruncate(file_handle_, offset + mapping_size)) return;
				file_size_ = offset + mapping_size;
			}
			char* real_data = static_cast<char*>(::mmap(
				0, offset - real_offset + mapping_size, PROT_READ | PROT_WRITE, MAP_SHARED,
				file_handle_, real_offset));
			if (data_ == MAP_FAILED) return;
#endif
			if (offset + mapping_size > file_size_)
			{
				file_size_ = offset + mapping_size;
			}
			data_ = real_data + (offset - real_offset.QuadPart);
			mapped_size_ = mapping_size;
			offset_ = offset;
		}

		bool writable_mmf::flush()
		{
			if (data_)
			{
				char* real_data = data_
					- (offset_ - offset_ / granularity_ * granularity_);
				size_t real_mapped_size = mapped_size_ + (data_ - real_data);
#if defined(_WIN32)
				return ::FlushViewOfFile(real_data, real_mapped_size) != 0
					&& FlushFileBuffers(file_handle_) != 0;
				if (::FlushViewOfFile(real_data, real_mapped_size) == 0) return false;
#else
				if (::msync(real_data, real_mapped_size, MS_SYNC) != 0) return false;
#endif
			}
#if defined(_WIN32)
			return FlushFileBuffers(file_handle_) != 0;
#else
			return true;
#endif
		}
	}
} // ns bsw
