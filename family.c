#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

#define MAX_CHILD 5

int
main(int argc, char *argv[])
{
    int num;
    int my_pid;
    int pid[MAX_CHILD];
  

    my_pid = getpid();
    printf(1, "The first Pid is : %d\n\n", my_pid);

    for(num = 0; num < MAX_CHILD; num++)
    {
        pid[num] = fork();
        if(pid[num] == 0)
            printf(1, "My Pid is : %d \n My father's Pid is : %d \n", getpid(), get_parent());
    }

    if(my_pid == getpid())
        printf(1, "\n My children are : %d \n",get_childrem(my_pid));

    exit();
}