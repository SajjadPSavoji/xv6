#include "types.h"
#include "stat.h"
#include "user.h"

#define NUMPROCS 10
#define MAXNUM 1000

int
main(int argc, char *argv[])
{
    int pid;

    for(int i=0; i < NUMPROCS; i++)
    {
        if((pid = fork()) < 0)
        {
            printf(0,"error occured!\n");
            exit();
        }
        if(pid == 0)
            break;
    }
    if(pid > 0)
        info();

    if(pid == 0)
    {
        for (int i = 0; i < MAXNUM; i++)
        {
            i*i*i*i*i*i;
        }
        
    }

    for (int i = 0; i < NUMPROCS; i++)
        wait();
    
    exit();
}
