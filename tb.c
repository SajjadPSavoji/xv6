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
    barrier_init(NUMFORK + 1);
    int pid;

    for(int i = 0; i < NUMFORK; i++)
        if((pid = fork()) == 0)
        {
            printf(1, "process with pid : %d has been forked and will sleep\n", getpid());
            sleep(100 * i);
            break;
        }
    
    printf(1, "process with pid : %d has reached the barrier\n", getpid());
    barrier();
    printf(1, "process with pid : %d has passed the barrier\n", getpid());
    for (int i = 0; i < NUMFORK; i++)
        wait();
    
    exit();
}