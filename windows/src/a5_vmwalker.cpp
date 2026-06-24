#include <windows.h>
#include <shlwapi.h>

#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdint.h>

#pragma comment(lib, "Shlwapi.lib")

inline bool TestSet(DWORD dwTarget, DWORD dwMask) {
    return ((dwTarget & dwMask) == dwMask);
}

#define SHOWMASK(dwTarget, type) \
    if (TestSet(dwTarget, PAGE_##type)) { std::cout << ", " << #type; }

void ShowProtection(DWORD dwTarget) {
    SHOWMASK(dwTarget, READONLY);
    SHOWMASK(dwTarget, GUARD);
    SHOWMASK(dwTarget, NOCACHE);
    SHOWMASK(dwTarget, READWRITE);
    SHOWMASK(dwTarget, WRITECOPY);
    SHOWMASK(dwTarget, EXECUTE);
    SHOWMASK(dwTarget, EXECUTE_READ);
    SHOWMASK(dwTarget, EXECUTE_READWRITE);
    SHOWMASK(dwTarget, EXECUTE_WRITECOPY);
    SHOWMASK(dwTarget, NOACCESS);
}

void WalkVM(HANDLE hProcess) {
    SYSTEM_INFO si;
    ZeroMemory(&si, sizeof(si));
    GetSystemInfo(&si);

    MEMORY_BASIC_INFORMATION mbi;
    ZeroMemory(&mbi, sizeof(mbi));

    LPCVOID pBlock = si.lpMinimumApplicationAddress;
    while (pBlock < si.lpMaximumApplicationAddress) {
        if (VirtualQueryEx(hProcess, pBlock, &mbi, sizeof(mbi)) == sizeof(mbi)) {
            LPCVOID pEnd = (PBYTE)pBlock + mbi.RegionSize;

            char szSize[MAX_PATH];
            StrFormatByteSizeA((LONGLONG)mbi.RegionSize, szSize, MAX_PATH);

            std::cout.fill('0');
            std::cout << std::hex << std::setw(16) << (uintptr_t)pBlock << "-"
                      << std::hex << std::setw(16) << (uintptr_t)pEnd << " ("
                      << szSize << ") ";

            switch (mbi.State) {
                case MEM_COMMIT:
                    std::cout << "Committed";
                    break;
                case MEM_FREE:
                    std::cout << "Free";
                    break;
                case MEM_RESERVE:
                    std::cout << "Reserved";
                    break;
            }

            if (mbi.Protect == 0 && mbi.State != MEM_FREE) {
                mbi.Protect = PAGE_READONLY;
            }
            ShowProtection(mbi.Protect);

            switch (mbi.Type) {
                case MEM_IMAGE:
                    std::cout << ", Image";
                    break;
                case MEM_MAPPED:
                    std::cout << ", Mapped";
                    break;
                case MEM_PRIVATE:
                    std::cout << ", Private";
                    break;
            }

            char szFilename[MAX_PATH];
            if (GetModuleFileNameA((HMODULE)pBlock, szFilename, MAX_PATH) > 0) {
                PathStripPathA(szFilename);
                std::cout << ", Module: " << szFilename;
            }

            std::cout << std::endl;
            pBlock = pEnd;
        } else {
            break;
        }
    }
}

void ShowVirtualMemory() {
    SYSTEM_INFO si;
    ZeroMemory(&si, sizeof(si));
    GetSystemInfo(&si);

    char szPageSize[MAX_PATH];
    StrFormatByteSizeA((LONGLONG)si.dwPageSize, szPageSize, MAX_PATH);

    uintptr_t minAddr = (uintptr_t)si.lpMinimumApplicationAddress;
    uintptr_t maxAddr = (uintptr_t)si.lpMaximumApplicationAddress;
    ULONGLONG memSize = (ULONGLONG)(maxAddr - minAddr);

    char szMemSize[MAX_PATH];
    StrFormatByteSizeA((LONGLONG)memSize, szMemSize, MAX_PATH);

    std::cout << "Virtual memory page size: " << szPageSize << std::endl;
    std::cout.fill('0');
    std::cout << "Minimum application address: 0x" << std::hex << std::setw(16)
              << minAddr << std::endl;
    std::cout << "Maximum application address: 0x" << std::hex << std::setw(16)
              << maxAddr << std::endl;
    std::cout << "Total available virtual memory: " << szMemSize << std::endl;
}

int main() {
    ShowVirtualMemory();
    WalkVM(GetCurrentProcess());
    getchar();
    return 0;
}
