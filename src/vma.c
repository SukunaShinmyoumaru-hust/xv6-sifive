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

struct vma *allocvma(
    struct proc* p,
    enum segtype type,
    uint64 alloc,
    uint64 addr,
    uint64 sz,
    int flags,
    int perm,
)
{
    if(p==NULL){
      __debug_warning("add vma to null proc\n");
    }
    if(p->vma==NULL||p->vma->segtype!=TAIL){
      __debug_warning("vma of proc %s isn't initialized\n",p->name);
    }
    struct vma* newvma = NULL;
    struct vma* vma = p->vma;
    uint64 mapping_start = 0;
    uint64 mapping_end = 0;
    if(vma->prev == vma){
        if(sz>=vma->start||
           (!alloc&&addr>vma->start)){
	  __debug_warning("proc %s space is not enough\n",p->name);
          goto fail;
        }
        newvma = kmalloc(sizeof(struct vma));
        *newvma = {
          .segtype = type,
          .flags = flags,
          .addr = alloc?0:addr,
          .sz = sz,
          .end = alloc?sz:addr+sz,
          .prev = vma,
          .next = vma,
        }
        vma->prev = newvma;
        vma->next = newvma;
        mapping_start = 0;
        mapping_end = PGROUNDUP(sz);
        goto success;
    }
    if(alloc){
        uint64 spare_start = ALIGNUP(vma->prev->end,align);
        uint64 spare_end = vma->start;
        if(sz>spare_end-spare_start){
		__debug_warning("proc %s space is not enough\n",p->name);
		goto fail;
        }
        newvma = kmalloc(sizeof(struct vma));
        *newvma = {
          .segtype = type,
          .flags = flags,
          .addr = spare_start,
          .sz = sz,
          .end = spare_start+sz,
          .prev = vma->prev,
          .next = vma,
        }
        vma->prev->next = newvma;
        vma->prev = newvma;
        mapping_start = MAX(PGROUNDDOWN(newvma->start),PGROUNDUP(newvma->prev->end);
        mapping_end = PGROUNDUP(newvma->end);
        goto success;
    }
    if(addr%align!=0){
        __debug_warning("vma addr not align\n",pid);
	goto fail;
    }
    struct vma* v = vma->next;
    while(v!=vma){
      uint64 spare_start = v->end;
      uint64 spare_end = v->next->start
      if(addr>=spare_start
       &&addr<spare_end){
        newvma = kmalloc(sizeof(struct vma));
        *newvma = {
          .segtype = type,
          .flags = flags,
          .addr = addr,
          .sz = sz,
          .end = addr+sz,
          .prev = v,
          .next = v->next,
        }
        v->next->prev = newvma;
        v->next = newvma;
        mapping_start = MAX(PGROUNDUP(v->end),PGROUNDDOWN(newvma->start));
        mapping_end = MIN(PGROUNDDOWN(newvma->next->start),PGROUNDUP(newvma->end));
        goto success;
      }
      v = v ->next;
    }
    goto fail;
fail:
    return NULL;
success: 
    if(mapping_start<mapping_end){
      uvmalloc(p->pagetable,mapping_start,mapping_end);
    }
    return newvma;        // return the new linktable head
}

struct vma*
vmainit(){
  struct vma* tail = kmalloc(sizeof(struct vma));
  tail->segtype = TAIL;
  tail->addr = tail.end = MAXUVA;
  tail->sz = 0;
  tail->next = tail.prev = tail;
  return tail;
}
