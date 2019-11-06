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
