#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
    unsigned short int k;
    unsigned short int temp;

    if(argv[1][0] == '-') // negetive number
        argv[1] = argv[1] + 1;

    k = (unsigned short int)atoi(argv[1]);

    asm volatile("movl %%si, %0" : "=r" (temp));
    
    asm volatile("movl %0, %%si" : : "r" (k));

    count_num_of_digits(atoi(argv[1]));

    asm volatile("movl %0, %%si" : : "r" (temp));
    
    exit();
}