#include <windows.h>
#include <stdio.h>

/* 第二种修改：先读取参数，再把 nClone 重新置 0，会覆盖参数效果 */
void StartClone(int nCloneID)
{
    TCHAR szFilename[MAX_PATH];
    GetModuleFileName(NULL, szFilename, MAX_PATH);

    TCHAR szCmdLine[MAX_PATH];
    sprintf(szCmdLine, "\"%s\" %d", szFilename, nCloneID);

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;

    BOOL bCreateOK = CreateProcess(
        szFilename,
        szCmdLine,
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si,
        &pi);

    if (bCreateOK)
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

int main(int argc, char* argv[])
{
    int nClone;

    if (argc > 1)
    {
        sscanf(argv[1], "%d", &nClone);
    }

    nClone = 0;

    printf("Process ID:%lu, Clone ID:%d\n", GetCurrentProcessId(), nClone);

    if (nClone < 5)
    {
        StartClone(++nClone);
    }

    getchar();
    return 0;
}
