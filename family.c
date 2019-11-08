#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

#define MAX_CHILD 3
#define WITH_GDC 1
#define NO_GDC 0

int
main(int argc, char *argv[])
{
    int num;
    int my_pid, pid1;
    int pid[MAX_CHILD];
  

    my_pid = getpid();
    printf(1, "The first Pid is : %d\n\n", my_pid);

    for(num = 0; num < MAX_CHILD; num++)
    {
        pid[num] = fork();
        if(pid[num] == 0)
        {
            printf(1, "My Pid is : %d \n My father's Pid is : %d \n", getpid(), get_parent());
            break;
        }
        else
            sleep(10);
    }
    dream(1);

    if(5 == getpid())
    {
        printf(1, "\n");
        for(num = 0; num < MAX_CHILD; num++)
        {
            pid1 = fork();
            if(pid1 == 0)
            {
                printf(1, "My Pid is : %d \n My father's Pid is : %d \n", getpid(), get_parent());
                break;
            }
            else
                sleep(10);
        }
    }
    dream(1);
    if(my_pid == getpid())
        printf(1, "\nMy children are : %d \n\n ", get_child(my_pid , NO_GDC));

    if(my_pid == getpid())
        printf(1, "My children are : %d \n ", get_child(my_pid , WITH_GDC));

    for (num = 0; num < MAX_CHILD; num++)
    {
        wait();
    }
    
    exit();
}