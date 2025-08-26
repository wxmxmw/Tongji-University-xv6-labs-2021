#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
int main(){

    int parent_fd[2];
    int child_fd[2];
    pipe(parent_fd);
    pipe(child_fd);
    char buf[8];
    if (fork()==0)
    {
        read(parent_fd[0], buf, sizeof buf);
        int id = getpid();   //Get the ID of the current process and print it
        printf("%d: received %s\n", id, buf);
        write(child_fd[1], "pong", 4);   //Write string "pong" to child_fd[1] pipe
    }
    else
    {
        int id = getpid();
        write(parent_fd[1], "ping", 4);  //Write the string "ping" to the parent_fd[1] pipe
        wait(0);
        read(child_fd[0], buf, sizeof buf);
        printf("%d: received %s\n", id, buf);
    }
    exit(0);
}