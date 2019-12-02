#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    if( argc != 3)
    {
        printf(2 , "test_q gets 2 arguments !\n");
        exit();
    }
    int pid , q;
    pid = atoi(argv[1]);
    q = atoi(argv[2]);
    change_q(pid , q);
    exit();
}
