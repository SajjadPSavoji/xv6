#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
char buf[128];
char sina[80000];
int
main(int argc, char *argv[])
{
    for (int i = 0; i < 80000; i++)
    {
        sina[i] += '0';
    }
    printf(1 , "done with the mem\n");
    exit();
}