#include <windows.h>
#include <stdio.h>
#include <string.h>

/* 使用同一个名字让父子进程能够访问同一个互斥体对象 */
static LPCTSTR g_szMutexName = "w2kdg.ProcTerm.mutex.Suicide";

/* 第二种修改：把等待方式改成 0，观察子进程不再阻塞等待 */
void StartClone(void)
{
    TCHAR szFilename[MAX_PATH];
    GetModuleFileName(NULL, szFilename, MAX_PATH);

    TCHAR szCmdLine[MAX_PATH];
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

        /* 等待时间改成 0，表示不阻塞等待，立即返回 */
        WaitForSingleObject(hMutexSuicide, 0);

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
