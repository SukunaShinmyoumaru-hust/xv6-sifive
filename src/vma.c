#include "include/param.h"
#include "include/types.h"
#include "include/memlayout.h"
#include "include/elf.h"
#include "include/riscv.h"
#include "include/pm.h"
#include "include/vm.h"
#include "include/vma.h"
#include "include/kalloc.h"
#include "include/proc.h"
#include "include/printf.h"
#include "include/string.h"

extern char trampoline[];
extern char sig_trampoline[];
extern char trapframe[];

struct vma *vma_list_init(struct proc *p)
{
  // alloc head
  struct vma *vma = (struct vma*)kmalloc(sizeof(struct vma));
  vma->next = vma->prev = vma;
  vma->type = NONE;
  p->vma = vma;

  // alloc TRAMPOLINE
  if(alloc_vma(p, TRAP, TRAMPOLINE, PGSIZE, PTE_R | PTE_X, 0, (uint64)trampoline) == NULL)
  {
    __debug_warn("[vma_list_init] TRAMPOLINE vma init fail\n");
    goto bad;
  }

  // alloc SIG_TRAMPOLINE
  if(alloc_vma(p, TRAP, SIG_TRAMPOLINE, PGSIZE, PTE_R | PTE_X, 0, (uint64)sig_trampoline) == NULL)
  {
    __debug_warn("[vma_list_init] SIG_TRAMPOLINE vma init fail\n");
    goto bad;
  }

  // alloc TRAPFRAME
  if(alloc_vma(p, TRAP, TRAPFRAME, PGSIZE, PTE_R | PTE_W, 0, (uint64)p->trapframe) == NULL)
  {
    __debug_warn("[vma_list_init] TRAPFRAME vma init fail\n");
    goto bad;
  }

  // alloc STACK
  if(alloc_vma(p, STACK, PGROUNDDOWN(USER_STACK_BOTTOM - 35 * PGSIZE), 35 * PGSIZE, PTE_R|PTE_W|PTE_U, 1, NULL) == NULL)
  {
    __debug_warn("[vma_list_init] stack vma init fail\n");
    goto bad;
  }

  // alloc MMAP
  if(alloc_mmap_vma(p, USER_MMAP_START, 0, 0, 0, 0) == NULL)
  {
    __debug_warn("[vma_list_init] mmap vma init fail\n");
    goto bad;
  }

  return vma;

bad:
  free_vma_list(p);
  return NULL;
}

struct vma *alloc_vma(
  struct proc *p,
  enum segtype type,
  uint64 addr,
  uint64 sz,
  int perm,
  int alloc,
  uint64 pa
)
{
  if(p == NULL)
  {
    __debug_warn("[alloc_vma] proc is null\n");
    return NULL;
  }

  uint64 start = PGROUNDDOWN(addr);
  uint64 end = addr + sz;
  end = PGROUNDUP(end);

  struct vma *vma_head = p->vma;
  struct vma *nvma = vma_head->next;

  while(nvma != vma_head)
  {
    if(end <= nvma->addr)
    {
      break;
    }
    else if (start >= nvma->end)
    {
      nvma = nvma->next;
    }
    else
    {
      __debug_warn("[alloc_vma] vma address overflow\n");
      return NULL;
    }
  }

  struct vma *vma = (struct vma*)kmalloc(sizeof(struct vma));
  vma->addr = start;
  vma->sz = sz;
  vma->end = end;
  vma->perm = perm;
  vma->f = NULL;
  vma->f_off = 0;

  vma->prev = nvma->prev;
  vma->next = nvma;
  nvma->prev->next = vma;
  nvma->prev = vma;

  if(sz != 0)
  {
    if(alloc == 1)
    {
      if(uvmalloc(p->pagetable, start, end, perm) != 0)
      {
        __debug_warn("[alloc_vma] uvmalloc start = %p, end = %p fail\n", start, end);
        goto bad;
      }
    }
    else
    {
      if(pa == 0)
      {
        __debug_warn("[alloc_vma] pa not valid\n");
        goto bad;
      }
      if(mappages(p->pagetable, start, sz, pa, perm) != 0)
      {
        __debug_warn("[alloc_vma] mappages failed\n");
        goto bad;
      }
    }
  }
  vma->type = type;
  return vma;

bad:
  kfree(vma);
  return NULL;
}


struct vma* type_locate_vma(struct vma *head, enum segtype type)
{
  struct vma *vma = NULL;

  if(type == LOAD)
  {
    vma = head->prev;
    while(vma != head)
    {
      if(vma->type == type)
      {
        return vma;
      }
      vma = vma->prev;
    }
  }
  else
  {
    vma = head->next;
    while(vma != head)
    {
      if(vma->type == type)
      {
        return vma;
      }
      vma = vma->next;
    }
  }
 
  return NULL;
}

struct vma *addr_locate_vma(struct vma*head, uint64 addr)
{
  struct vma *vma = head->next;
  while(vma != head)
  {
    if(vma->addr <= addr && addr < vma->end)
    {
      return vma;
    }
    vma = vma->next;
  }
  return NULL;
}

struct vma* alloc_mmap_vma(struct proc *p, uint64 addr, uint64 sz, int perm, struct file *f ,uint64 f_off)
{
  struct vma *mvma = type_locate_vma(p->vma, MMAP);
  if(addr == 0)
  {
    addr = PGROUNDDOWN(mvma->addr - sz);
  }
  struct vma *vma = alloc_vma(p, MMAP, addr, sz, perm, 1, NULL);
  vma->f = f;
  vma->f_off = f_off;
  return vma;
}

struct vma *alloc_stack_vma(struct proc *p, uint64 addr, int perm)
{
  struct vma *vma = type_locate_vma(p->vma, STACK);
  uint64 start = PGROUNDDOWN(addr);
  uint64 end = vma->addr;
  vma->addr = start;
  if(start < USER_STACK_TOP)
  {
    __debug_warn("[alloc_stack_vma] stack address illegal\n");
    return NULL;
  }
  vma->sz += (end - start);
  if(uvmalloc(p->pagetable, start, end, perm) != 0)
  {
    __debug_warn("[alloc_stack_vma] stack vma alloc fail\n");
    return NULL;
  }
  return vma;
}
 
struct vma *alloc_addr_heap_vma(struct proc *p, uint64 addr, int perm)
{
  struct vma *vma = type_locate_vma(p->vma, HEAP);
  struct vma *lvma = type_locate_vma(p->vma, LOAD);
  if(vma == NULL)
  {
    uint64 start = lvma->end;
    if(start > addr)
    {
      __debug_warn("[alloc_addr_heap_vma] addr illegal\n");
      return NULL;
    }
    vma = alloc_vma(p, HEAP, start, addr - start, perm, 1, NULL);
    return vma;
  }
  else
  {
    if(lvma->end > addr)
    {
      __debug_warn("[alloc_addr_heap_vma] addr illegal\n");
      return NULL;
    }
    
    if(uvmalloc(p->pagetable, vma->end, PGROUNDUP(addr), perm) != 0)
    {
      __debug_warn("[alloc_addr_heap_vma] uvmalloc fail\n");
      return NULL;
    }
    vma->end = PGROUNDUP(addr);
    vma->sz = (vma->end - vma->addr);
    return vma;
  }
}

struct vma *alloc_sz_heap_vma(struct proc *p, uint64 sz, int perm)
{
  struct vma *vma = type_locate_vma(p->vma, HEAP);
  struct vma *lvma = type_locate_vma(p->vma, LOAD);
  if(vma == NULL)
  {
    uint64 start = lvma->end;
    if(start + sz < start)
    {
      __debug_warn("[alloc_sz_heap_vma] addr illegal\n");
      return NULL;
    }
    vma = alloc_vma(p, HEAP, start, sz, perm, 1, NULL);
    return vma;
  }
  
  if(sz != 0)
  {
    if(vma->end + sz < lvma->end)
    {
      __debug_warn("[alloc_sz_heap_vma] addr illegal\n");
      return NULL;
    }
    
    if(vma->end + sz > vma->end)
    {
      if(uvmalloc(p->pagetable, vma->end, PGROUNDUP(vma->end + sz), perm) != 0)
      {
        __debug_warn("[alloc_addr_heap_vma] uvmalloc fail\n");
        return NULL;
      }
    }
    else
    {
      if(uvmdealloc(p->pagetable, PGROUNDDOWN(vma->end + sz), vma->end) != 0)
      {
        __debug_warn("[alloc_addr_heap_vma] uvmdealloc fail\n");
        return NULL;
      }
    }
    vma->end = PGROUNDUP(vma->end + sz);
    vma->sz = vma->end - vma->addr;
  }
  return vma;
}

struct vma *alloc_load_vma(struct proc *p, uint64 addr, uint64 sz, int perm)
{
  return alloc_vma(p, LOAD, addr, sz, perm, 1, NULL);
}

int free_vma_list(struct proc *p)
{
  struct vma *vma_head = p->vma;
  struct vma *vma = vma_head->next;
  while(vma != vma_head)
  {
    uint64 a;
    pte_t *pte;
    for(a = vma->addr; a < vma->end; a += PGSIZE){
      if((pte = walk(p->pagetable, a, 0)) == 0)
        continue;
      if((*pte & PTE_V) == 0)
        continue;
      if(PTE_FLAGS(*pte) == PTE_V)
        continue;
      uint64 pa = PTE2PA(*pte);
      freepage((void*)pa);
      *pte = 0;
    }
    vma = vma->next;
    kfree(vma->prev);
  }
  kfree(vma);
  return 1;
}

int free_vma(struct proc *p, uint64 addr, uint64 len)
{
  struct vma *a = addr_locate_vma(p->vma, addr);
  struct vma *b = addr_locate_vma(p->vma, addr + len);
  if(!a || !b || a != b)
  {
    __debug_warn("[free_vma] no vma matched\n");
    return 0;
  }
  if(uvmdealloc(p->pagetable, a->addr, a->end) != 0)
  {
    __debug_warn("[free_vma] uvmdealloc fail\n");
    return 0;
  }
  return 1;
}

void vma_copy(struct proc *np, struct vma *head)
{
  struct vma *nvma_head = (struct vma *)kmalloc(sizeof(struct vma));
  nvma_head->next = nvma_head->prev = nvma_head;
  nvma_head->type = NONE;
  struct vma *pvma = head->next;
  while(pvma != head)
  {
    struct vma *nvma = (struct vma *)kmalloc(sizeof(struct vma));
    memmove(nvma, pvma, sizeof(struct vma));
    nvma->next = nvma->prev = NULL;
    nvma->prev = nvma_head->prev;
    nvma->next = nvma_head;
    nvma_head->prev->next = nvma;
    nvma_head->prev = nvma;
    pvma = pvma->next;
  }
}

static char * vma_type[] = {
  [NONE]  "NONE",
  [LOAD]  "LOAD",
  [TEXT]  "TEXT",
  [DATA]  "DATA",
  [BSS]   "BSS",
  [HEAP]  "HEAP",
  [STACK] "STACK",
  [TRAP]  "TRAP",
  [MMAP]  "MMAP",
};


void print_vma_info(struct vma *head)
{
  struct vma * pvma = head->next;
  __debug_info("\t\taddr\t\t\tsz\t\t\tend\t\ttype\n");
  while(pvma != head){
    __debug_info("[vma_info]%p\t%p\t%p\t%s\n", 
                pvma->addr, pvma->sz, pvma->end, vma_type[pvma->type]);
    pvma = pvma->next;
  }
}
  
