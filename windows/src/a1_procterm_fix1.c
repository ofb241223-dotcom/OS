#include <windows.h>
#include <stdio.h>
#include <string.h>

/* 使用同一个名字让父子进程能够访问同一个互斥体对象 */
static LPCTSTR g_szMutexName = "w2kdg.ProcTerm.mutex.Suicide";

/* 第一种修改：把 child 改成别的字符串，观察新进程不再进入 Child 分支 */
void StartClone(void)
{
    TCHAR szFilename[MAX_PATH];
    GetModuleFileName(NULL, szFilename, MAX_PATH);

    /* 这里故意把 child 改成 abc，用于验证参数判断失效后的现象 */
    TCHAR szCmdLine[MAX_PATH];
    sprintf(szCmdLine, "\"%s\" abc", szFilename);

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
        printf("Child quiting.\n");
        CloseHandle(hMutexSuicide);
    }
}

int main(int argc, char* argv[])
{
    /* 只有参数恰好是 child 时，才会进入子进程逻辑 */
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
