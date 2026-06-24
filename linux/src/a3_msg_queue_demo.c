#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MSGKEY 80
#define MSG_SIZE 300

struct msgform {
    long mtype;
    char mtext[MSG_SIZE];
} msg;

int msgqid;
int i;

void client_process(void) {
    int result;

    msgqid = msgget(MSGKEY, 0777);
    if (msgqid == -1) {
        perror("client msgget");
        exit(1);
    }

    for (i = 0; i < 10; i++) {
        printf("(Client) sent message %d\n", i + 1);
        result = msgsnd(msgqid, &msg, MSG_SIZE, 0);
        if (result == -1) {
            perror("client msgsnd");
            exit(1);
        }
    }

    exit(0);
}

void server_process(void) {
    int getsize;

    msgqid = msgget(MSGKEY, 0777);
    if (msgqid == -1) {
        perror("server msgget");
        exit(1);
    }

    for (i = 0; i < 10; i++) {
        getsize = (int)msgrcv(msgqid, &msg, MSG_SIZE, 0, 0);
        if (getsize == -1) {
            perror("server msgrcv");
            exit(1);
        }

        printf("(Server) received %d bytes, content: %s\n", getsize, msg.mtext);
    }

    /* 删除消息队列，便于观察 msgctl(IPC_RMID) 的作用。 */
    msgctl(msgqid, IPC_RMID, 0);
    exit(0);
}

int main(void) {
    msgqid = msgget(MSGKEY, 0777 | IPC_CREAT);
    if (msgqid == -1) {
        perror("main msgget");
        return 1;
    }

    /* mtype 必须大于 0。 */
    msg.mtype = 1;
    strcpy(msg.mtext, "hello from client");

    while ((i = fork()) == -1) {
    }
    if (i == 0) {
        server_process();
    }

    while ((i = fork()) == -1) {
    }
    if (i == 0) {
        client_process();
    }

    wait(0);
    wait(0);
    return 0;
}
