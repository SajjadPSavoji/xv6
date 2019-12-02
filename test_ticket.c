#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    if( argc != 3)
    {
        printf(2 , "test_tikect gets 2 arguments !\n");
        exit();
    }
    int pid , tickets;
    pid = atoi(argv[1]);
    tickets = atoi(argv[2]);
    change_ticket(pid , tickets);
    exit();
}
