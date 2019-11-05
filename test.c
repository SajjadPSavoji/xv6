#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
    long int k;
    k = (long int)atoi(argv[1]);
    asm volatile("movl %0, %%ecx" : : "r" (k));
    // asm(
    //     "movl k, %ax \n"
    //     // "movl %ax, kk \n"
    // );
    // printf(1, "%d \n", k);
    count_num_of_digits(atoi(argv[1]));
    exit();
}