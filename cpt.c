#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
char buf[128];
void
signle_argument(char* argv[]){
    int fd2;
    int n;
    if ((fd2 = open(argv[1] , O_WRONLY) )< 0){
        fd2 = open(argv[1] , O_CREATE);
        close(fd2);
        if((fd2 = open(argv[1] , O_WRONLY)) < 0){
            printf(2,"cpt:after creation of file , write error\n");
        }
    }
    while((n = read(1, buf, sizeof(buf))) > 0) {
        if (write(fd2, buf, n) != n) {
            printf(1, "cpt: write error\n");
            close(fd2);
            exit();
        }
    }
    if(n < 0){
        printf(1, "cpt: read error\n");
        close(fd2);
        exit();
    }
    close(fd2);
}
void 
double_argument(char* argv[]){
    int fd1 , fd2;
    int n;
    fd1 = open(argv[1] , O_RDONLY);
    if ((fd2 = open(argv[2] , O_WRONLY) )< 0){
        fd2 = open(argv[2] , O_CREATE);
        close(fd2);
        if((fd2 = open(argv[2] , O_WRONLY)) < 0){
            printf(2,"cpt:after creation of file , write error\n");
        }
    }
    while((n = read(fd1, buf, sizeof(buf))) > 0) {
        if (write(fd2, buf, n) != n) {
            printf(1, "cpt: write error\n");
            close(fd1);close(fd2);
            exit();
        }
    }
    if(n < 0){
        printf(1, "cpt: read error\n");
        close(fd1);close(fd2);
        exit();
    }
    close(fd1);close(fd2);
}
int
main(int argc, char *argv[])
{
    switch (argc)
    {
    case 1:
        printf(2,"cpt take 1 or 2 arguments");
        break;
    case 2:
        signle_argument(argv);
        break;
    case 3:
        double_argument(argv);
        break;
    default:
        //error
        printf(2,"cpt takes at most 2 args");
        break;
    }
    exit();
}
