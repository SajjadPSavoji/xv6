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
    printf(1, "now will call system call \n");
    acq();
    printf(1, "double acquire done \n");
    exit();
}