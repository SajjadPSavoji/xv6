#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
    short int k;
    k = (int)atoi(argv[1]);
    asm volatile("movl %0, %%si" : : "r" (k));

    count_num_of_digits(atoi(argv[1]));
    exit();
}