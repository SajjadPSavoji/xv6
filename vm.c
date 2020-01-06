#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"
#include "fcntl.h"

extern char data[];  // defined by kernel.ld
pde_t *kpgdir;  // for use in scheduler()

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void
seginit(void)
{
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X|STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  lgdt(c->gdt, sizeof(c->gdt));
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if(*pde & PTE_P){
    pgtab = (pte_t*)P2V(PTE_ADDR(*pde));
  } else {
    if(!alloc || (pgtab = (pte_t*)kalloc()) == 0)
      return 0;
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;

  a = (char*)PGROUNDDOWN((uint)va);
  last = (char*)PGROUNDDOWN(((uint)va) + size - 1);
  for(;;){
    if((pte = walkpgdir(pgdir, a, 1)) == 0)
      return -1;
    if(*pte & PTE_P)
      panic("remap");
    *pte = pa | perm | PTE_P;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
//
// setupkvm() and exec() set up every page table like this:
//
//   0..KERNBASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (PHYSTOP)
// (directly addressable from end..P2V(PHYSTOP)).

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap {
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
 { (void*)KERNBASE, 0,             EXTMEM,    PTE_W}, // I/O space
 { (void*)KERNLINK, V2P(KERNLINK), V2P(data), 0},     // kern text+rodata
 { (void*)data,     V2P(data),     PHYSTOP,   PTE_W}, // kern data+memory
 { (void*)DEVSPACE, DEVSPACE,      0,         PTE_W}, // more devices
};

// Set up kernel part of a page table.
pde_t*
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if((pgdir = (pde_t*)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE);
  if (P2V(PHYSTOP) > (void*)DEVSPACE)
    panic("PHYSTOP too high");
  for(k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if(mappages(pgdir, k->virt, k->phys_end - k->phys_start,
                (uint)k->phys_start, k->perm) < 0) {
      freevm(pgdir);
      return 0;
    }
  return pgdir;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void
kvmalloc(void)
{
  kpgdir = setupkvm();
  switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void
switchkvm(void)
{
  lcr3(V2P(kpgdir));   // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void
switchuvm(struct proc *p)
{
  if(p == 0)
    panic("switchuvm: no process");
  if(p->kstack == 0)
    panic("switchuvm: no kstack");
  if(p->pgdir == 0)
    panic("switchuvm: no pgdir");

  pushcli();
  mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
                                sizeof(mycpu()->ts)-1, 0);
  mycpu()->gdt[SEG_TSS].s = 0;
  mycpu()->ts.ss0 = SEG_KDATA << 3;
  mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
  // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
  // forbids I/O instructions (e.g., inb and outb) from user space
  mycpu()->ts.iomb = (ushort) 0xFFFF;
  ltr(SEG_TSS << 3);
  lcr3(V2P(p->pgdir));  // switch to process's address space
  popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void
inituvm(pde_t *pgdir, char *init, uint sz)
{
  char *mem;

  if(sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W|PTE_U);
  memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int
loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
  uint i, pa, n;
  pte_t *pte;

  if((uint) addr % PGSIZE != 0)
    panic("loaduvm: addr must be page aligned");
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, addr+i, 0)) == 0)
      panic("loaduvm: address should exist");
    pa = PTE_ADDR(*pte);
    if(sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if(readi(ip, P2V(pa), offset+i, n) != n)
      return -1;
  }
  return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int
allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  char *mem;
  uint a;

  if(newsz >= KERNBASE)
    return 0;
  if(newsz < oldsz)
    return oldsz;

  a = PGROUNDUP(oldsz);
  for(; a < newsz; a += PGSIZE){
    mem = kalloc();
    if(mem == 0){
      cprintf("allocuvm out of memory\n");
      deallocuvm(pgdir, newsz, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    if(mappages(pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
      cprintf("allocuvm out of memory (2)\n");
      deallocuvm(pgdir, newsz, oldsz);
      kfree(mem);
      return 0;
    }
  }
  return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int
deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  pte_t *pte;
  uint a, pa;

  if(newsz >= oldsz)
    return oldsz;

  a = PGROUNDUP(newsz);
  for(; a  < oldsz; a += PGSIZE){
    pte = walkpgdir(pgdir, (char*)a, 0);
    if(!pte)
      a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
    else if((*pte & PTE_P) != 0){
      pa = PTE_ADDR(*pte);
      if(pa == 0)
        panic("kfree");
      char *v = P2V(pa);
      kfree(v);
      *pte = 0;
    }
  }
  return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void
freevm(pde_t *pgdir)
{
  uint i;

  if(pgdir == 0)
    panic("freevm: no pgdir");
  deallocuvm(pgdir, KERNBASE, 0);
  for(i = 0; i < NPDENTRIES; i++){
    if(pgdir[i] & PTE_P){
      char * v = P2V(PTE_ADDR(pgdir[i]));
      kfree(v);
    }
  }
  kfree((char*)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void
clearpteu(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if(pte == 0)
    panic("clearpteu");
  *pte &= ~PTE_U;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  char *mem;

  if((d = setupkvm()) == 0)
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");

    if((!(*pte & PTE_P)) && (!(*pte & PTE_PG)))
      panic("copyuvm: page not present");

    if((*pte & PTE_PG))
    {
      // copy pte*.page file
      // @impliment : is in the fork 
      // -------------------
      flags = PTE_FLAGS(*pte);
      if(mappages(d, (void*)i, PGSIZE, V2P(*pte), flags) < 0) {
        goto bad;
      }
      continue;
    }

    // else :
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char*)P2V(pa), PGSIZE);
    if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
      kfree(mem);
      goto bad;
    }
  }
  return d;

bad:
  freevm(d);
  return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char*
uva2ka(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if((*pte & PTE_P) == 0)
    return 0;
  if((*pte & PTE_U) == 0)
    return 0;
  return (char*)P2V(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int
copyout(pde_t *pgdir, uint va, void *p, uint len)
{
  char *buf, *pa0;
  uint n, va0;

  buf = (char*)p;
  while(len > 0){
    va0 = (uint)PGROUNDDOWN(va);
    pa0 = uva2ka(pgdir, (char*)va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (va - va0);
    if(n > len)
      n = len;
    memmove(pa0 + (va - va0), buf, n);
    len -= n;
    buf += n;
    va = va0 + PGSIZE;
  }
  return 0;
}

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.

// -----------------------------------------------------------------------------

int fix_paging(pde_t* pgdir)
{
  uint i;
  char *mem;

  struct proc *curproc = myproc();
  uint sz = curproc->sz;

  if(sz >= MAX_TOTAL_PAGES * PAGESZ)
    return -1;
  if(sz <= MAX_PYSC_PAGES * PAGESZ)
    return 0;

  // allocate a dumy mem to copy page content
  // only becuase its the only way i know
  // allocate one memory page in the physical main memory
  // and memset it on each iteration
  if((mem = kalloc()) == 0)
  {
    cprintf("mem could not be allocated in fixpages\n");
    return -1;

  }

  // (sz-2)* PGSIZE becuase i assume that the last 2 pages are ustack and kstack
  for(i = MAX_PYSC_PAGES *PGSIZE; i < sz; i += PGSIZE){
    // clear mem
    if(page_out(i, mem, pgdir)<0)
      panic("page out failed\n");
  }
  // unalloc dummy mem
  kfree(mem);
  // on success
  return 0;
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// this functions initializes proc->pages recording
// NOTE: should be called after fix_paging
void init_proc_pages()
{
  uint i;
  int j;

  struct proc *curproc = myproc();
  uint sz = curproc->sz;

  int lim = 0;
  curproc->index_page = 0;


  if(sz >= MAX_TOTAL_PAGES * PAGESZ)
    panic("init proc pages: should have checked in fix_paging");

  if(sz <= MAX_PYSC_PAGES * PAGESZ)
    lim = sz;

  if(sz >  MAX_PYSC_PAGES * PAGESZ)
    lim =  MAX_PYSC_PAGES * PAGESZ;

  

  for(j = 0 , i = 0; i < lim; i += PGSIZE , j++){
      curproc->pages[j].va = PGROUNDDOWN(i);
      // curproc->pages[j].age  = 0;
      // curproc->pages[j].freq = 0;

      // add any other fileds should be added here
  }
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------

void
pgflt_handler(void)
{
  // @impliment:
  struct proc* curproc = myproc();
  uint cr2  = rcr2();
  uint va;
  char* mem;

  //add one page fault 
  curproc->total_num_pgflts += 1;

  va = PGROUNDDOWN(cr2);

  page_out_handler();

  mem = kalloc();
  if(mem == 0){
    panic("pg fault handler:  out of memory\n");
  }
  memset(mem, 0, PGSIZE);
  page_in(va , mem);

  // // read file
  // char buff[50];
  // page_path(buff, curproc->pid, (uint)va, 50);
  // if((fd = fs_open(buff , O_RDONLY))< 0)
  //   panic("pg fault handler: can not open .page file\n");

  // fs_read(fd , PGSIZE , mem);

  // if(fs_close(fd) < 0)
  //   panic("pg fault handler: can not close .page file\n");

  // // delete .page file 
  // begin_op();
  // if (fs_unlink(buff))
  //   panic("pg fault handler: can not delete .page file\n");
  // end_op();

  // // map new page
  // if(mappages(pgdir, (char*)va, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
  //   panic("pg fault handler: out of fucking memory\n");
  // }

  // do page out

  // cprintf("my pid is: %d\n" , curproc->pid);
  // cprintf("page address : %x\n" , va);
  // cprintf("pte_addr : %x\n" , PTE_ADDR(va));
  // cprintf("v2p : %x\n" , V2P(va));
  ////////////////////////////////////////////
}

// -----------------------------------------------------------------
// this function impliments a pageout given pre allocated  mem
// and cp->pgdir , and the virtual memory i(va)
// it also updates pte and pde flags and writes a .page file to 
// the /_pages/cp->pid/i.page
// it also frees the kernel memory previously occupied by the va
int
page_out(uint i, char* mem, pde_t* pgdir)
{
  struct proc* curproc = myproc();
  pte_t *pte;
  uint pa;

  // i = PGROUNDDOWN(i);
  // cprintf("aaaaaaaaaaaaaaa, %x ,%x\n",i, PGROUNDDOWN(i));
  memset(mem , 0 , PGSIZE);

  if((pte = walkpgdir(pgdir, (char*) i, 0)) == 0)
    panic("fixpaging: pte should exist");


  if(!(((*pte) & PTE_P)))
    panic("fixgpaginf: page not present");

  pa = PTE_ADDR(*pte);
  // flags = PTE_FLAGS(*pte);
  
  memmove(mem, (char*)P2V(pa), PGSIZE);
  // free the page residing on physical memory
  kfree((char*)P2V(pa));

  // update PTE : set PAGEOUT flag and clear PRESENT flag
  *pte = (*pte | PTE_PG )& ~PTE_P;

  // store mem into the disk
  // make strings
  cprintf("paged out %x\n" ,i);
  char buff[50];
  page_path(buff, curproc->pid, (uint)i, 50);
  cprintf("buff:: %s!!!\n" , buff);

  // make file descriptor
  int fd = fs_open(buff , O_CREATE | O_WRONLY);
  if(fd < 0)
  {
    cprintf("kir1\n");
    return -1;
  }
  if(fs_write(fd , PGSIZE , mem) < 0)
  {

    cprintf("kir2\n");
    return -1;

  }
  if(fs_close(fd) < 0)
  {
    cprintf("kir3\n");
    return -1;
  }
  return 0;
}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// cp->index will be added
// mem should not be freed afterward
int
page_in(uint va , char* mem)
{
  struct proc* curproc = myproc();
  pde_t* pgdir = curproc->pgdir;
  int fd;

  // make approp path
  char buff[50];
  page_path(buff, curproc->pid, (uint)va, 50);

  // open va.page
  if((fd = fs_open(buff , O_RDONLY))< 0)
    panic("pg fault handler: can not open .page file\n");
  
  // rem from va.page 
  fs_read(fd , PGSIZE , mem);
  if(fs_close(fd) < 0)
    panic("pg fault handler: can not close .page file\n");

  // delete .page file 
  begin_op();
  if (fs_unlink(buff))
    panic("pg fault handler: can not delete .page file\n");
  end_op();

  // map new page
  if(mappages(pgdir, (char*)va, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
    panic("pg fault handler: out of fucking memory\n");
  }

  // update cp->pages
  curproc->pages[curproc->index_page].va = va;
  // curproc->pages[curproc->index_page].age = 0;
  // curproc->pages[curproc->index_page].freq = 0;

  // add any other fields
  curproc->index_page = (curproc->index_page + 1)%MAX_PYSC_PAGES;

  cprintf("%x paged in\n" , va);
  return 0;
}
// -------------------------------------------------------------------

// -------------------------------------------------------------------
void 
page_out_handler()
{

  #ifdef NONE
  panic("i am rnning : NONE");
  #endif

  #ifdef FIFO
  page_sched_fifo();
  cprintf("i am rnning : FIFO\n");
  #endif

  #ifdef LRU
  page_sched_lru();
  cprintf("i am rnning : LRU\n");
  #endif

  #ifdef CLOCK
  page_sched_clock();
  cprintf("i am rnning : CLOCK\n");
  #endif

  #ifdef NFU
  page_sched_nfu();
  cprintf("i am rnning : NFU\n");
  #endif

}
// -------------------------------------------------------

// schedulers --------------------------------------------
void page_sched_fifo(void)
{
  // init mem
  char* mem;
  mem = kalloc();
  if(mem == 0){
    panic("pg fault handler:  out of memory\n");
  }
  memset(mem, 0, PGSIZE);

  struct proc* curproc = myproc();

  page_out(curproc->pages[curproc->index_page].va, mem, curproc->pgdir);
  // kfree(mem);
}

// np->index should be updated for page_in
void page_sched_lru(void)
{
  // // init mem
  // char* mem;
  // mem = kalloc();
  // if(mem == 0){
  //   panic("pg fault handler:  out of memory\n");
  // }
  // memset(mem, 0, PGSIZE);

  // struct proc* curproc = myproc();

  // int max_idx = -1;
  // int max_age = -1;

  // for (int i = 0; i < MAX_PYSC_PAGES; i++)
  // {
  //   if(curproc->pages[i].age > max_age)
  //   {
  //     max_age = curproc->pages[i].age;
  //     max_idx = i;
  //   }
  // }

  // // for page in
  // curproc->index_page  = max_idx;
  
  // page_out(curproc->pages[curproc->index_page].va, mem, curproc->pgdir);
}

void page_sched_nfu(void)
{
  // // init mem
  // char* mem;
  // mem = kalloc();
  // if(mem == 0){
  //   panic("pg fault handler:  out of memory\n");
  // }
  // memset(mem, 0, PGSIZE);

  // struct proc* curproc = myproc();
  // int min_idx  =  0;
  // int min_freq = __INT32_MAX__;

  // for (int i = 0; i < MAX_PYSC_PAGES; i++)
  // {
  //   if(curproc->pages[i].freq < min_freq)
  //   {
  //     min_freq = curproc->pages[i].freq;
  //     min_idx = i;
  //   }
  // }

  // // for page in
  // curproc->index_page  = min_idx;
  
  // page_out(curproc->pages[curproc->index_page].va, mem, curproc->pgdir);
}

void page_sched_clock(void)
{
}
// ----------------------------------------------------------------------