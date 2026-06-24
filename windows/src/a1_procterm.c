#include <windows.h>
#include <stdio.h>
#include <string.h>

static LPCTSTR g_szMutexName = "w2kdg.ProcTerm.mutex.Suicide";

void StartClone(void)
{
    TCHAR szFilename[MAX_PATH];
    GetModuleFileName(NULL, szFilename, MAX_PATH);

    TCHAR szCmdLine[MAX_PATH];
    /* 实验 1-3 步骤 3：将 child 改成别的字符串后重新编译执行。 */
    sprintf(szCmdLine, "\"%s\" child", szFilename);

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

void Parent(void)
{
    HANDLE hMutexSuicide = CreateMutex(
        NULL,
        TRUE,
        g_szMutexName);

    if (hMutexSuicide != NULL)
    {
        printf("Creating the child process.\n");
        StartClone();

        printf("Telling the child process to quit.\n");
        getchar();

        ReleaseMutex(hMutexSuicide);
        CloseHandle(hMutexSuicide);
    }
}

void Child(void)
{
    HANDLE hMutexSuicide = OpenMutex(
        SYNCHRONIZE,
        FALSE,
        g_szMutexName);

    if (hMutexSuicide != NULL)
    {
        printf("Child waiting for suicide instructions.\n");

        WaitForSingleObject(hMutexSuicide, INFINITE);
        /* 实验 1-3 步骤 4：改为 WaitForSingleObject(hMutexSuicide, 0) 后重新编译执行。 */

        printf("Child quiting.\n");
        CloseHandle(hMutexSuicide);
    }
}

int main(int argc, char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "child") == 0)
    {
        Child();
    }
    else
    {
        Parent();
    }

    return 0;
}
