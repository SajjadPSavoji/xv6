#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "date.h"


int
main(int argc, char *argv[])
{
    int t1;
    int t2;

    t1 = get_time();
    printf(1,"now time is: %d\n" ,t1);

    dream(100);

    t2 = get_time();
    printf(1,"now time is: %d\n" ,t2);
    
    printf(1,"I have slept for : %d\n" ,t2 - t1);

    exit();
}