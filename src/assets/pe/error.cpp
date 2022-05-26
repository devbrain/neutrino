#include "predef.h"


#if defined(PREDEF_PLATFORM_WIN32)
#include <Windows.h>
#else
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#endif
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include "error.hpp"
#include "string.hpp"

namespace bsw
{
    std::string error_to_string (long err_code)
    {
        std::string res;
#if defined(PREDEF_PLATFORM_WIN32)
        LPVOID lpMsgBuf;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | 
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err_code,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL );
        res.assign ((LPCTSTR)lpMsgBuf);
        LocalFree (lpMsgBuf);
#else
        char *s;
        size_t size;

        size = 1024;
        s = (char*)malloc(size);
        if (s == NULL)
        {
            return NULL;
        }
        strerror_r ((int)err_code, s, size);
        res.assign (s);
        free (s);
#endif
        return trim (res);
    }
    // -------------------------------------------------------------------------------------
    void raise_system_error (const char* fn_name, const char* src, int line_num)
    {
#if defined(PREDEF_PLATFORM_WIN32)
        long error_code = GetLastError ();
#else
        long error_code = errno;
#endif
        raise_system_error (fn_name, src, line_num, error_code);
    }
    // ---------------------------------------------------------------------------------------------
    void raise_system_error (const char* fn_name, const char* src, int line_num, long error_code)
    {
        std::ostringstream os;
        os << "Function: " << fn_name << " failed with error code "
#if defined(PREDEF_PLATFORM_WIN32)
            << "0x" 
            << std::hex << std::uppercase << std::setw (sizeof(error_code)) << std::setfill('0') << error_code
#else
            << error_code
#endif
            << "\n"
            << "Reason:  " << error_to_string (error_code) << "\n"
            << "At: " << src << ":" << std::dec << line_num;
        throw std::runtime_error (os.str ());
    }
} // ns bsw


