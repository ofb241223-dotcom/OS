#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(void)
{
    int x;

    /* 用当前时间初始化随机数种子，便于每次运行时得到不同的等待时间 */
    srand((unsigned)time(NULL));

    /* 如果 fork 失败，就一直重试，直到创建出子进程为止 */
    while ((x = fork()) == -1)
    {
    }

    if (x == 0)
    {
        /* 子进程随机等待 0~1 秒，然后输出 a */
        sleep(rand() % 2);
        printf("a\n");
    }
    else
    {
        /* 父进程随机等待 0~2 秒，然后输出 b */
        sleep(rand() % 3);
        printf("b\n");
    }

    /* 父子进程最后都会各自输出一次 c */
    printf("c\n");
    return 0;
}
