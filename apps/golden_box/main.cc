#include <fstream>
#include <iostream>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>

static ULONG_PTR GetParentProcessId() // By Napalm @ NetCore2K
{
    ULONG_PTR pbi[6];
    ULONG ulSize = 0;
    LONG (WINAPI *NtQueryInformationProcess)(HANDLE ProcessHandle, ULONG ProcessInformationClass,
            PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength); 
    *(FARPROC *)&NtQueryInformationProcess = 
        GetProcAddress(LoadLibraryA("NTDLL.DLL"), "NtQueryInformationProcess");
    if(NtQueryInformationProcess){
        if(NtQueryInformationProcess(GetCurrentProcess(), 0,
                    &pbi, sizeof(pbi), &ulSize) >= 0 && ulSize == sizeof(pbi))
            return pbi[5];
    }
    return (ULONG_PTR)-1;
}

static void _windows_init_console(int argc, char **argv) {
    (void)argc, (void)argv;
    ULONG_PTR ppid = GetParentProcessId();
    if(ppid == (ULONG_PTR)-1) {
        AllocConsole();
    } else {
        AttachConsole(ppid);
    }

    freopen("CONIN$", "r", stdin); 
    freopen("CONOUT$", "w", stdout); 
    freopen("CONOUT$", "w", stderr); 
}

int main(int argc, char* argv[]) {

    // Just before the event loop
    //AllocConsole();
    ULONG_PTR ppid = GetParentProcessId();
    if(ppid == (ULONG_PTR)-1) {
        AllocConsole();
    } else {
        AttachConsole(ppid);
    }

    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    int hCrt = _open_osfhandle((intptr_t) handle_out, _O_TEXT);
    FILE* hf_out = _fdopen(hCrt, "w");
    setvbuf(hf_out, NULL, _IONBF, 1);
    *stdout = *hf_out;

    HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
    hCrt = _open_osfhandle((intptr_t) handle_in, _O_TEXT);
    FILE* hf_in = _fdopen(hCrt, "r");
    setvbuf(hf_in, NULL, _IONBF, 128);
    *stdin = *hf_in;
    
  // _windows_init_console(argc, argv);
    std::cout << "ZOPA" << std::endl;
    //int x;
    //std::cin >> x;
    return 0;
}