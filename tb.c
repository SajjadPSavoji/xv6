#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

char buf[128];

#define NUMFORK 5

int
main(int argc, char *argv[])
{
    int pid;
    barrier_init(10);
    for(int i = 0; i < NUMFORK; i++)
        if((pid = fork()) == 0)
        {
            printf(1, "process with pid : %d has been forked and will sleep\n", getpid());
            sleep(100 * i);
            break;
        }
    
    printf(1, "process with pid : %d has reached the barrier\n", getpid());
    barrier();
    if(pid != 0)
        printf(1, "now the processes will pass the barriers\n");    
    printf(1, "pid : %d \n", getpid());
    for (int i = 0; i < NUMFORK; i++)
        wait();
    
    exit();
}