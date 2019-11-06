#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}
/////////
int
sys_sina(void)
{
  struct file *f;
  f=myproc()->ofile[0];
  return filewrite(f,  "sina\n", sizeof("sina\n"));
}

int
sys_count_num_of_digits(void)
{
  unsigned short int n;
  int num_of_digits = 1;
  char temp;

  struct file *f;
  f = myproc() -> ofile[1];

  asm volatile("movl %%si, %0" : "=r" (n));

  while(1)
  {
    n = n/10;
    if((n) > 0)
    {
      num_of_digits++;
    }
    else
      break;
  }
  temp = '0' + num_of_digits;
  
  filewrite(f,  &temp, sizeof(temp));
  filewrite(f,  "\n", sizeof("\n"));
  return 1;
}

int
sys_set_path(void)
{
  // file to print stuff in console
  struct file *f;
  f=myproc()->ofile[2];
  // clear up all prev paths
  for (int i = 0; i < MAX_PATHS; i++)
  {
    for (int j = 0; j < MAX_PATH; j++)
    {
      Path[i][j] = NULL;
    }
  }

  // read string passed to set path from stack
  char* new_paths;
  if(argstr(0 , &new_paths) < 0)
    return filewrite(f,  "str read in kernel error\n", 25);

  // good to go
  // parse paths and add new path
  char* cur = new_paths;
  int path_num = 0;
  int path_idx = 0;
  for (int chert = 0 ; chert <= strlen(new_paths); chert++)
  {
    if(cur[0] == DELIM){
      Path[path_num][path_idx] = NULL;
      path_num ++;
      path_idx = 0;
      cur ++;
      continue;
    }
    Path[path_num][path_idx] = cur[0];
    cur++;
    path_idx++;
  }
  // print Path for test
  // for (int i = 0; i < MAX_PATHS; i++)
  // {
  //   filewrite(f,  Path[i], 10);
  // }
  return 1;
}

int
sys_dream(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  // acquire(&tickslock);
  n = n* 100;
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      // release(&tickslock);
      return -1;
    }
    // sleep(&ticks, &tickslock);
  }
  // release(&tickslock);
  return 0;
}

int
sys_get_time(void)
{
  struct rtcdate t1;
  cmostime(&t1);
  return t1.second;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
