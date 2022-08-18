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
#include "include/riscv.h"
#include "include/mmap.h"

struct vma *vma_list_init(struct proc *p)
{
  if(p == NULL)
  {
    __debug_warn("[vma_list_init] proc is NULL\n");
    return NULL;
  }
  // alloc head
  struct vma *vma = (struct vma*)kmalloc(sizeof(struct vma));
  if(vma == NULL)
  {
    __debug_warn("[vma_list_init] vma kmalloc failed\n");
    return NULL;
  }
  vma->next = vma->prev = vma;
  vma->type = NONE;
  p->vma = vma;
  
  // printf("[vma list init] trapframe map\n");
  // alloc TRAPFRAME
  if(alloc_vma(p, TRAP, TRAPFRAME, PGSIZE, PTE_R | PTE_W , 0, (uint64)p->trapframe) == NULL)
  {
    __debug_warn("[vma_list_init] TRAPFRAME vma init fail\n");
    goto bad;
  }
  
  // printf("[vma list init] stack map\n");
  // alloc STACK
  if((vma = alloc_vma(p, STACK, PGROUNDDOWN(USER_STACK_BOTTOM - 10 * PGSIZE), 10 * PGSIZE, PTE_R|PTE_W|PTE_U, 1, NULL)) == NULL)
  {
    __debug_warn("[vma_list_init] stack vma init fail\n");
    goto bad;
  }
  // 30 pages not mapped
  vma->addr = vma->addr - 30 * PGSIZE;

  // printf("[vma list init] mmap map\n");
  // alloc MMAP
  if(alloc_mmap_vma(p, 0, USER_MMAP_START, 0, 0, 0, 0) == NULL)
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
  if(vma == NULL)
  {
    __debug_warn("[alloc_vma] vma kmalloc failed\n");
    return NULL;
  }
  
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
    else if(pa != 0)
    {
      if(mappages(p->pagetable, start, sz, pa, perm) != 0)
      {
        __debug_warn("[alloc_vma] mappages failed\n");
        goto bad;
      }
    }
  }
  
  vma->addr = start;
  vma->sz = sz;
  vma->end = end;
  vma->perm = perm;
  vma->fd = -1;
  vma->f_off = 0;
  vma->flags = 0;
  vma->mmap = 0;
  vma->type = type;

  vma->prev = nvma->prev;
  vma->next = nvma;
  nvma->prev->next = vma;
  nvma->prev = vma;
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

struct vma *part_locate_vma(struct vma *head, uint64 start, uint64 end)
{
  struct vma *a = addr_locate_vma(head, start);
  struct vma *b = addr_locate_vma(head, end-1);
  if(!a || !b || a != b)
  {
    __debug_warn("[part_locate_vma] start = %p, end = %p, not found\n", start, end);
    return NULL;
  }
  return a;
}

struct vma* alloc_mmap_vma(struct proc *p, int flags, uint64 addr, uint64 sz, int perm, int fd ,uint64 f_off)
{
  struct vma *vma = NULL;

  struct vma *mvma = type_locate_vma(p->vma, MMAP);
  if(addr == 0)
  {
    addr = PGROUNDDOWN(mvma->addr - sz);
    // __debug_info("[alloc_mmap_vma] addr = %p\n", addr);
  }

  vma = alloc_vma(p, MMAP, addr, sz, perm, 1, NULL);
  if(vma == NULL)
  {
    __debug_warn("[alloc_mmap_vma] alloc failed\n");
    return NULL;
  }

  vma->fd = fd;
  vma->f_off = f_off;
  vma->flags = flags;
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
  addr = PGROUNDUP(addr);
  if(vma == NULL)
  {
    uint64 start = lvma->end;
    uint64 sz = 0;
    if(start < addr)sz = addr - start ;
    vma = alloc_vma(p, HEAP, start, sz, perm, 1, NULL);
    return vma;
  }
  else
  {
    if(lvma->end > addr)
    {
      __debug_warn("[alloc_addr_heap_vma] addr %p illegal\n", addr);
      return vma;
    }
    if(vma->end > addr)
    {
      if(uvmdealloc(p->pagetable, addr, vma->end) != 0)
      {
        __debug_warn("[alloc_addr_heap_vma] uvmdealloc fail\n");
        return vma;
      }
      vma->end = addr;
      vma->sz = (vma->end - vma->addr);
      return vma;
    }
    
    if(uvmalloc(p->pagetable, vma->end, addr, perm) != 0)
    {
      __debug_warn("[alloc_addr_heap_vma] uvmalloc fail\n");
      return vma;
    }
    vma->end = addr;
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
  if(vma_head == NULL)
  {
    return 1;
  }
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
      //__debug_warn("[free single vma]free:%p\n",pa);
      freepage((void*)pa);
      //__debug_warn("[free vma list]free end\n");
      *pte = 0;
    }
    vma = vma->next;
    kfree(vma->prev);
  }
  kfree(vma);
  p->vma = NULL;
  return 1;
}

struct vma *addr_sz_locate_vma(struct vma*head, uint64 addr, uint64 sz)
{
  if(head == NULL)
  {
    __debug_warn("[addr_sz_locate_vma] head is nil\n");
    return NULL;
  }
  struct vma *vma = head->next;
  while(vma != head)
  {
    if(vma->addr == addr && vma->sz == sz)
    {
      return vma;
    }
    vma = vma->next;
  }
  return NULL;
}

int free_vma(struct proc *p, struct vma *del)
{
  if(del == NULL)
  {
    __debug_warn("[free_vma] del is nil\n");
    return 0;
  }
  if(del->prev == NULL || del->next == NULL)
  {
    __debug_warn("[free_vma] del is illegal\n");
    return 0;
  }
  
  struct vma *prev = del->prev;
  struct vma *next = del->next;
  prev->next = next;
  next->prev = prev;
  del->next = del->prev = NULL;
  if(uvmdealloc(p->pagetable, del->addr, del->end) != 0)
  {
    __debug_warn("[free_vma] uvmdealloc fail\n");
    return 0;
  }
  kfree(del);
  return 1;
}

struct vma* vma_copy(struct proc *np, struct vma *head)
{
  struct vma *nvma_head = (struct vma *)kmalloc(sizeof(struct vma));
  if(nvma_head == NULL)
  {
    __debug_warn("[vma_copy] nvma_head kmalloc failed\n");
    goto err;
  }
  nvma_head->next = nvma_head->prev = nvma_head;
  nvma_head->type = NONE;
  np->vma = nvma_head;
  struct vma *pvma = head->next;
  while(pvma != head)
  {
    struct vma *nvma = NULL;
    if(pvma->type == TRAP)
    {
      if((nvma = alloc_vma(np, TRAP, TRAPFRAME, PGSIZE, PTE_R | PTE_W , 0, (uint64)np->trapframe)) == NULL)
      {
        __debug_warn("[vma_list_init] TRAPFRAME vma init fail\n");
        goto err;
      }
    }
    else
    {
      nvma = (struct vma *)kmalloc(sizeof(struct vma));
      if(nvma == NULL)
      {
        __debug_warn("[vma_copy] nvma kmalloc failed\n");
        goto err;
      }
      memmove(nvma, pvma, sizeof(struct vma));
      nvma->next = nvma->prev = NULL;
      nvma->prev = nvma_head->prev;
      nvma->next = nvma_head;
      nvma_head->prev->next = nvma;
      nvma_head->prev = nvma;
    }
    pvma = pvma->next;
  }
  
  return nvma_head;
  
err:
  np->vma = NULL;
  __debug_warn("[vm_copy] failed\n");
  free_vma_list(np);
  return NULL;
}

int vma_deep_mapping(pagetable_t old, pagetable_t new, const struct vma *vma)
{
  uint64 start;
  if(vma->type == STACK)
  {
    start = vma->end - vma->sz;
  }
  else
  {
    start = vma->addr;
  }
  pte_t *pte;
  uint64 pa;
  char *mem;
  long flags;
  
  while(start < vma->end)
  {
    if((pte = walk(old, start, 0)) == NULL)
    {
      panic("uvmcopy: pte should exist");
    }
    if((*pte & PTE_V) == 0)
    {
      panic("uvmcopy: page not present");
    }
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);

    mem = (char *)allocpage();

    if(mem == NULL)
    {
      __debug_warn("[vma deep mapping] alloc page fail\n");
      goto err;
    }

    memmove(mem, (char *)pa, PGSIZE);

    if(mappages(new, start, PGSIZE, (uint64)mem, flags) != 0)
    {
      __debug_warn("[vma_deep_mapping] start = %p, pa = %p\n", start, mem);
      freepage(mem);
      goto err;
    }

    start += PGSIZE;
  }
  return 0;
  
err:
  vmunmap(new, vma->addr, (start - vma->addr) / PGSIZE, 1);
  return -1;
}

int vma_shallow_mapping(pagetable_t old, pagetable_t new, const struct vma *vma)
{
  uint64 start;
  if(vma->type == STACK)
  {
    start = vma->end - vma->sz;
  }
  else
  {
    start = vma->addr;
  }
  uint64 pa;
  pte_t *pte;
  long flags;

  while(start < vma->end)
  {
    if((pte = walk(old, start, 0)) == NULL)
    {
      panic("uvmcopy: pte should exist");
    }
    if((*pte & PTE_V) == 0)
    {
      panic("uvmcopy: page not present");
    }
    pa = PTE2PA(*pte);
    flags = PTE_FLAGS(*pte);

    if(mappages(new, start, PGSIZE, pa, flags) != 0)
    {
      __debug_warn("[vma_shallow_mapping] start = %p, pa = %p\n", start, pa);
      goto err;
    }
    start +=PGSIZE;
  }
  return 0;

err:
  vmunmap(new, vma->addr, (start - vma->addr) / PGSIZE, 1);
  return -1;
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
uint64
growproc(int n)
{
  struct proc *p = myproc();
  //printf("[growproc]proc name:%s\n",p->name);
  //printf("[growproc]want grow to %p\n",n);
  struct vma* vma = alloc_addr_heap_vma(p, n, PTE_R|PTE_W|PTE_U);
  if(vma == NULL){
    __debug_warn("[growproc]alloc heap not found\n");
    return 0;
  }
  //printf("[growproc]actually grow to %p\n",vma->end);
  return vma->end;
}

uint64 growprocsize(uint64 sz)
{
  struct proc *p = myproc();
  struct vma *vma = alloc_sz_heap_vma(p, sz, PTE_R|PTE_W|PTE_U);
  if(vma == NULL)
  {
    __debug_warn("[growproc] alloc heap failed\n");
    return 0;
  }
  return vma->end - PGROUNDUP(sz);
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


void print_vma_info(struct proc* p)
{
  struct vma * head = p->vma;
  struct vma * pvma = head->next;
  __debug_info("\t\tva\t\t\tpa\t\t\tsz\t\t\tend\t\ttype\n");
  while(pvma != head){
    __debug_info("[vma_info]%p\t%p\t%p\t%p\t%s\n", 
                pvma->addr, kwalkaddr1(p->pagetable,pvma->addr),pvma->sz, pvma->end, vma_type[pvma->type]);
    pvma = pvma->next;
  }
}

void print_single_vma(pagetable_t pagetable,struct vma* v)
{
  __debug_info("[vma_info]va %p\tsz %p\tend %p\tname %s\n", 
                v->addr,v->sz, v->end, vma_type[v->type]);

}
  
