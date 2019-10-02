#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    printf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    printf(1, "%s %d %d %d\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf(1, "ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf(1, "ls: cannot stat %s\n", buf);
        continue;
      }
      printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}
char buf[128];
void
signle_argument(char* argv[]){

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
    printf(2,"%d",fd1);printf(2,"%d",fd2);
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
