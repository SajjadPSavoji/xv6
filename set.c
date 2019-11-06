#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
    if(argc < 2)
    {
        printf(2, "misssing argument\n");
        exit();
    }

    set_path(argv[1]);
    exit();
}