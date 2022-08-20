#include "include/param.h"
#include "include/types.h"
#include "include/memlayout.h"
#include "include/elf.h"
#include "include/riscv.h"
#include "include/pm.h"
#include "include/vm.h"
#include "include/kalloc.h"
#include "include/proc.h"
#include "include/printf.h"
#include "include/string.h"
#include "sifive/platform.h"
#include "include/errno.h"
#include "include/mmap.h"

/*
 * the kernel's page table.
 */
pagetable_t kernel_pagetable;
extern char etext[];  // kernel.ld sets this to end of kernel code.
extern char trampoline[]; // trampoline.S
extern char sig_trampoline[]; //sig_trampoline.S

void
kvminit()
{
  kernel_pagetable = (pagetable_t) allocpage();
  // printf("kernel_pagetable: %p\n", kernel_pagetable);

  memset(kernel_pagetable, 0, PGSIZE);

  // CLINT
  kvmmap(CLINT_V, CLINT, 0x10000, PTE_R | PTE_W);

  // PLIC
  kvmmap(PLIC_V, PLIC, 0x400000, PTE_R | PTE_W);
  #ifdef RAM
  kvmmap(RAMDISK, RAMDISK, 0x40000000, PTE_R | PTE_W);
  #endif
  #ifdef SD
  // SPI
  kvmmap(SPI2_CTRL_ADDR, SPI2_CTRL_ADDR_P, SPI2_CTRL_SIZE, PTE_R | PTE_W);
  #endif
  
  // map kernel text executable and read-only.
  kvmmap(KERNBASE, KERNBASE, (uint64)etext - KERNBASE, PTE_R|PTE_X);
  // map kernel data and the physical RAM we'll make use of.
  kvmmap((uint64)etext, (uint64)etext, PHYSTOP - (uint64)etext, PTE_R | PTE_W);
  // map the trampoline for trap entry/exit to
  // the highest virtual address in the kernel.
  kvmmap(TRAMPOLINE, (uint64)trampoline, PGSIZE, PTE_R | PTE_X);
  // map the trapoline for signal
  kvmmap(SIG_TRAMPOLINE, (uint64)sig_trampoline, PGSIZE, PTE_R | PTE_X | PTE_U);
  
  __debug_info("kvminit\n");
  print_free_page_n();
}

// Switch h/w page table register to the kernel's page table,
// and enable paging.
void
kvminithart()
{
  w_satp(MAKE_SATP(kernel_pagetable));
  // reg_info();
  sfence_vma();
  //printf("[kvm]readtime:%p\n",*(uint64*)(PLIC_V));
  __debug_info("kvminithart\n");
}

// add a mapping to the kernel page table.
// only used when booting.
// does not flush TLB or enable paging.
void
kvmmap(uint64 va, uint64 pa, uint64 sz, int perm)
{
  if(mappages(kernel_pagetable, va, sz, pa, perm) != 0)
    panic("kvmmap");
}

// translate a kernel virtual address to
// a physical address. only needed for
// addresses on the stack.
// assumes va is page aligned.
uint64
kvmpa(uint64 va)
{
  return kwalkaddr(kernel_pagetable, va);
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned. Returns 0 on success, -1 if walk() couldn't
// allocate a needed page-table page.
int
mappages(pagetable_t pagetable, uint64 va, uint64 size, uint64 pa, int perm)
{
  uint64 a, last;
  pte_t *pte;

  a = PGROUNDDOWN(va);
  last = PGROUNDDOWN(va + size - 1);

  for(;;){
    if((pte = walk(pagetable, a, 1)) == NULL)
      return -1;
    if(*pte & PTE_V){
      //printf("pagetable:%p map va:%p to pa:%p\n",pagetable,va,pa);
      //panic("remap");
      *pte = PA2PTE(pa) | perm | PTE_V | PTE_A | PTE_D;
      return 0;
    }
    *pte = PA2PTE(pa) | perm | PTE_V | PTE_A | PTE_D;
    if(a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  
  return 0;
}

// Remove npages of mappings starting from va. va must be
// page-aligned. The mappings must exist.
// Optionally free the physical memory.
void
vmunmap(pagetable_t pagetable, uint64 va, uint64 npages, int do_free)
{
  uint64 a;
  pte_t *pte;

  if((va % PGSIZE) != 0)
    panic("vmunmap: not aligned");

  for(a = va; a < va + npages*PGSIZE; a += PGSIZE){
    if((pte = walk(pagetable, a, 0)) == 0)
      panic("vmunmap: walk");
    if((*pte & PTE_V) == 0)
      panic("vmunmap: not mapped");
    if(PTE_FLAGS(*pte) == PTE_V)
      panic("vmunmap: not a leaf");
    if(do_free){
      uint64 pa = PTE2PA(*pte);
      if(pa==NULL){
        printf("NULL pte:%p\n",pte);

      }
      freepage((void*)pa);
    }
    *pte = 0;
  }
}

pte_t *
walk(pagetable_t pagetable, uint64 va, int alloc)
{
  if(va >= MAXVA)
    panic("walk");

  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pagetable[PX(level, va)];
    if(*pte & PTE_V) {
      pagetable = (pagetable_t)PTE2PA(*pte);
    } else {
      if(!alloc || (pagetable = (pde_t*)allocpage()) == NULL)
        return NULL;
      
      memset(pagetable, 0, PGSIZE);
      *pte = PA2PTE(pagetable) | PTE_V;
    }
  }
  return &pagetable[PX(0, va)];
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
uint64
walkaddr(pagetable_t pagetable, uint64 va)
{
  pte_t *pte;
  uint64 pa;

  if(va >= MAXVA)
    return NULL;

  pte = walk(pagetable, va, 0);
  if(pte == 0)
    return NULL;
  if((*pte & PTE_V) == 0)
    return NULL;
  if((*pte & PTE_U) == 0)
    return NULL;
  pa = PTE2PA(*pte);
  return pa;
}

// Look up a virtual address, return the physical address,
// or 0 if not mapped.
// Can only be used to look up user pages.
//extend permission
uint64
experm(pagetable_t pagetable, uint64 va,uint64 perm)
{
  pte_t *pte;
  uint64 pa;

  if(va >= MAXVA)
    return NULL;

  pte = walk(pagetable, va, 0);
  if(pte == 0)
    return NULL;
  if((*pte & PTE_V) == 0)
    return NULL;
  if((*pte & PTE_U) == 0)
    return NULL;
  *pte |= perm;
  pa = PTE2PA(*pte);
  return pa;
}


uint64
kwalkaddr(pagetable_t kpt, uint64 va)
{
  uint64 off = va % PGSIZE;
  pte_t *pte;
  uint64 pa;
  
  pte = walk(kpt, va, 0);
  if(pte == 0)
    panic("kvmpa");
  if((*pte & PTE_V) == 0)
    panic("kvmpa");
  pa = PTE2PA(*pte);
  return pa+off;
}



uint64
kwalkaddr1(pagetable_t kpt, uint64 va)
{
  uint64 off = va % PGSIZE;
  pte_t *pte;
  uint64 pa;
  
  pte = walk(kpt, va, 0);
  if(pte == 0)
    return NULL;
  if((*pte & PTE_V) == 0)
    return NULL;
  pa = PTE2PA(*pte);
  return pa+off;
}

// Allocate PTEs and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
uint64
uvmalloc(pagetable_t pagetable, uint64 start, uint64 end, int perm)
{
  char *mem;
  uint64 a;
  if(start>=end)return -1;
  for(a = start; a < end; a += PGSIZE){
    mem = allocpage();
    if(mem == NULL){
      uvmdealloc(pagetable, start, a);
      printf("uvmalloc kalloc failed\n");
      return -1;
    }
    memset(mem, 0, PGSIZE);
    if (mappages(pagetable, a, PGSIZE, (uint64)mem, perm) != 0) {
      freepage(mem);
      uvmdealloc(pagetable, start, a);
      printf("[uvmalloc]map user page failed\n");
      return -1;
    }
  }
  return 0;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
uint64
uvmdealloc(pagetable_t pagetable, uint64 start, uint64 end)
{
  
  if(start>=end)return -1;
  if(PGROUNDUP(start) < PGROUNDUP(end)){
    int npages = (PGROUNDUP(end) - PGROUNDUP(start)) / PGSIZE;
    vmunmap(pagetable, PGROUNDUP(start), npages, 1);
  }

  return 0;
}

// Recursively free page-table pages.
// All leaf mappings must already have been removed.
void
freewalk(pagetable_t pagetable)
{
  //printf("[freewalk]enter %p\n",pagetable);
  // there are 2^9 = 512 PTEs in a page table.
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];
    if(pagetable[i]==kernel_pagetable[i])continue;
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X) ) == 0){
      // this PTE points to a lower-level page table.
      uint64 child = PTE2PA(pte);
      freewalk((pagetable_t)child);
      pagetable[i] = 0;
    } 
    else if(pte & PTE_V){
      __debug_warn("freewalk: leaf %d\n",i);
      //pagetable[i]=0;
    }
  }
  freepage((void*)pagetable);
}

// create an empty user page table.
// returns 0 if out of memory.
pagetable_t
kvmcreate()
{
  pagetable_t pagetable;
  pagetable = (pagetable_t) allocpage();
  if(pagetable == NULL)
    return NULL;
  memset(pagetable, 0, PGSIZE);
  memmove(pagetable, kernel_pagetable, PGSIZE);
  return pagetable;
}

// Free user memory pages,
// then free page-table pages.
void
uvmfree(struct proc *p)
{
  free_vma_list(p);
  // vmunmap(pagetable, 0, PGROUNDUP(sz)/PGSIZE, 1);
  freewalk(p->pagetable);
}


int 
uvmprotect(uint64 va, uint64 len, int perm)
{
  int fence = 0;
  uint64 i;
  pte_t *pte = NULL;
  struct proc *p = myproc();
  if(va % PGSIZE != 0)
  {
    __debug_warn("[uvmprotect] va is illegal\n");
    return -EINVAL;
  }

  if(len % PGSIZE)
  {
    __debug_warn("[uvmprotect] len is illegal\n");
    return -EINVAL;
  }
  
  struct vma *vma = (struct vma*)find_map_fix(p, va, len);
  if(!vma) vma = part_locate_vma(p->vma, va, va + len);
  if(vma == NULL)
  {
    __debug_warn("[uvmprotect] memory illegal\n");
    return -EINVAL;
  }
  
  if(vma->type == MMAP && vma->fd != -1)
  {
    struct file *f = p->ofile[vma->fd];
    if(f == NULL)
    {
      __debug_warn("[uvmprotect] something wrong in MMAP memoryn\n");
      return -EINVAL;
    }
    if(!(f->writable) && (perm & PTE_W))
    {
      return -EINVAL;
    }
  }
  
  for(i = 0; i < len; i += PGSIZE)
  {
    pte = walk(p->pagetable, va + i, 0);
    if(pte == NULL)
    {
      __debug_warn("[uvmprotect] page not mapped\n");
      return -ENOMEM;
    }
    if(*pte & PTE_V)
    {
      fence = 1;
      *pte = (*pte & ~(PTE_W | PTE_R | PTE_X)) | perm;
      // *pte |= perm;
    }
    else
    {
      __debug_warn("[uvmprotect] page is invalid\n");
      return -ENOMEM;
    }
  }
  if(fence)
    sfence_vma();
  return 0;
}

static int handle_stack_page_fault(struct vma *vma, uint64 stval)
{
  struct proc *p = myproc();
  stval = PGROUNDDOWN(stval);
  uint64 end = vma->end - vma->sz;
  if(uvmalloc(p->pagetable, stval, end, PTE_R | PTE_W | PTE_U) < 0)
  {
    __debug_warn("[handle_stack_page_fault] stack malloc failed\n");
    return -1;
  }
  vma->sz += end - stval;
  return 0;
}

int handle_page_fault(int kind, uint64 stval)
{
  struct proc *p = myproc();
  // __debug_info("[handle page fault] kind = %d stval = %p\n", kind, stval);
  // print_vma_info(p);
  
  struct vma* vma = addr_locate_vma(p->vma, stval);
  if(!vma)
  {
    __debug_info("[handle_page_fault] vma not found, stval = %p\n", stval);
    return -1;
  }

  pte_t *pte = walk(p->pagetable, stval, 0);
  if(pte)
  {
    if(*pte & PTE_V)
    {
      if(kind == 1 && !(*pte & PTE_W))
      {
        // __debug_info("[handle_page_fault] permission not match\n");
        return -1;
      }
    }
  }

  switch (vma->type)
  {
    case STACK:
      return handle_stack_page_fault(vma, stval);
      break;
    default:
      break;
  }

  return 0;
}
