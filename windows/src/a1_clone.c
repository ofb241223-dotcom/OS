#include <windows.h>
#include <stdio.h>

/* 创建一个新的“克隆进程”，并把新的克隆编号传给它 */
void StartClone(int nCloneID)
{
    /* 取到当前 exe 自己的完整路径，后面用它再次启动自己 */
    TCHAR szFilename[MAX_PATH];
    GetModuleFileName(NULL, szFilename, MAX_PATH);

    /* 组装子进程的命令行，例如："clone.exe" 3 */
    TCHAR szCmdLine[MAX_PATH];
    sprintf(szCmdLine, "\"%s\" %d", szFilename, nCloneID);

    /* 启动信息结构，CreateProcess 调用前需要先清零并设置大小 */
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    /* 用来接收新创建子进程的句柄和线程信息 */
    PROCESS_INFORMATION pi;

    /* 启动一个新的进程，并让它在新的控制台窗口中运行 */
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
        /* 当前进程不再使用这些句柄，及时关闭 */
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

int main(int argc, char* argv[])
{
    /* nClone 表示“当前进程是第几号克隆体” */
    int nClone = 0;
    /* 修改语句：int nClone; */

    /* 第一种修改：nClone = 0; */
    if (argc > 1)
    {
        /* 把命令行中的字符串数字转换成整数，存入 nClone */
        sscanf(argv[1], "%d", &nClone);
    }

    /* 第二种修改：nClone = 0; */
    printf("Process ID:%lu, Clone ID:%d\n", GetCurrentProcessId(), nClone);

    /* 只有克隆编号小于 5 时，才继续创建下一个子进程 */
    if (nClone < 5)
    {
        /* ++nClone 先加 1，再把新的编号传给子进程 */
        StartClone(++nClone);
    }

    /* 暂停当前窗口，便于观察输出结果 */
    getchar();
    return 0;
}
