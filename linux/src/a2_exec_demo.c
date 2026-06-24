#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    pid_t pid;

    /* 先创建一个子进程 */
    pid = fork();

    if (pid < 0)
    {
        /* fork 返回负数，说明创建子进程失败 */
        fprintf(stderr, "Fork Failed\n");
        return 1;
    }
    else if (pid == 0)
    {
        /* 子进程用 /bin/ls 替换自己原来的代码 */
        execlp("/bin/ls", "ls", NULL);

        /* 如果 execlp 执行失败，后面的代码才会继续运行 */
        fprintf(stderr, "execlp failed\n");
        return 1;
    }
    else
    {
        /* 父进程等待子进程结束，再输出提示信息 */
        wait(NULL);
        printf("Child Complete\n");
    }

    return 0;
}
