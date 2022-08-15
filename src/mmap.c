#include "include/vma.h"
#include "include/proc.h"
#include "include/cpu.h"
#include "include/printf.h"
#include "include/mmap.h"
#include "include/types.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/vm.h"
#include "include/kalloc.h"
#include "include/string.h"
#include "include/errno.h"

map_fix* do_mmap_fix(uint64 start, uint64 len, int flags, int fd, off_t offset)
{
    struct proc* p = myproc();
    struct file* f = fd < 0 ? NULL : p->ofile[fd];
    map_fix* mf = kmalloc(sizeof(map_fix));
    *mf = (map_fix){
        .addr = start,  //映射起始地址
        .sz = len,
        .mmap = (uint64)f,
        .fd = fd,
        .flags = flags,
        .f_off = offset,
        .next = p->mf,
        .type = MMAP
    };
    p->mf = mf;
    return mf;
}

uint64 do_mmap(uint64 start, uint64 len, int prot, int flags, int fd, off_t offset)
{
    struct proc *p = myproc();
    struct vma *vma = NULL;

    if(flags & MAP_ANONYMOUS)
    {
        fd = -1;
        goto ignore_fd;
    }

    if(fd < 0)
    {
        __debug_warn("[do_mmap] fd illegal, fd = %d\n", fd);
        return -1;
    }

    if(fd > NOFILEMAX(p))
    {
        __debug_warn("[do_mmap] fd illegal, fd(%d) > NOFILEMAX(%d)\n", fd, NOFILEMAX(p));
        return -1;
    }

ignore_fd:
    if(offset < 0)
    {
        __debug_warn("[do_mmap] offset illegal, offset = %d\n", offset);
        return -1;
    }
    if(start % PGSIZE != 0)
    {
        __debug_warn("[do_mmap] mmap start address not aligned\n");
        return -1;
    }
    // print_vma_info(p);
    // __debug_info("[dp_mmap] start = %p, len = %p, flags = %p, fd = %d, offset = %d\n", start, len, flags, fd, offset);
    int perm = PTE_U;
    if(prot & PROT_READ) 
        perm  |= (PTE_R | PTE_A);
    if(prot & PROT_WRITE)
        perm  |= (PTE_W | PTE_D);
    if(prot & PROT_EXEC)
        perm  |= (PTE_X | PTE_A);

    struct file *f = fd == -1 ? NULL : p->ofile[fd];
    if(fd != -1 && f == NULL)
    {
        __debug_warn("[do_mmap] mmap file illegal\n");
        return -1;
    }

    if((flags & MAP_FIXED) && start != 0)
    {
        vma = (struct vma *)do_mmap_fix(start, len, flags, fd, offset);
        goto skip_vma;
    }

    vma = alloc_mmap_vma(p, flags, start, len, perm, fd, offset);
    start = vma->addr;
    if(vma == NULL)
    {
        __debug_warn("[do_mmap] alloc mmap vma failed\n");
        return -1;
    }

    uint64 mmap_sz ;
skip_vma:
    mmap_sz = 0;
    if(fd != -1)
    {
        mmap_sz = f->ep->file_size - offset;
        if(len < mmap_sz)
            mmap_sz = len;
        f->off = offset;
    }
    else
    {
        vma->mmap = NULL;
        if (flags & MAP_SHARED) {
            vma->mmap |= MMAP_SHARE_FLAG;
        }
        vma->mmap |= MMAP_ANONY_FLAG;
        vma->f_off = 0;
        return start;
    }

    vma->mmap = (uint64)filedup(f);
    if(!(flags & MAP_SHARED))
    {
        return start;
    }
    vma->mmap |= MMAP_SHARE_FLAG;
    
    // read and copy file to memory
    uint64 end_pagespace = mmap_sz % PGSIZE;
    int page_n = PGROUNDUP(mmap_sz) >> PGSHIFT;
    uint64 va = start;

    for(int i = 0; i < page_n; ++i)
    {
        uint64 pa = experm(p->pagetable, va, perm);
        if(pa == NULL)
        {
            __debug_warn("[do_mmap] va = %p, pa not found\n", va);
            return -1;
        }
        if(i != page_n - 1)
        {
            fileread(f, va, PGSIZE);
        }
        else 
        {
            fileread(f, va, end_pagespace);
            memset((void *)(pa + end_pagespace), 0, PGSIZE - end_pagespace);
        }
        va += PGSIZE;
    }

    return start;
}

map_fix* do_munmap_fix(struct proc* p,uint64 start, uint64 len){
    map_fix* i = p->mf;
    map_fix* last = NULL;
    while(i){
        if(i->addr==start&&i->sz == len){
            goto success;
        }
        last = i;
        i = i->next;
    }
    return NULL;
success:
    if(last)last->next = i->next;
    else p->mf = i->next;
    return i;
}

map_fix * find_map_fix(struct proc *p, uint64 start, uint64 len)
{
  map_fix *i = p->mf;
  while(i)
  {
    if(i->addr == start && len <= i->sz)
    {
      return i;
    }
    i = i->next;
  }
  return NULL;
}

uint64 do_munmap(struct proc* np,uint64 start, uint64 len)
{
    struct proc *p = np?np:myproc();
    map_fix* mf = do_munmap_fix(p,start,len);
    struct vma *vma = mf ? mf : addr_sz_locate_vma(p->vma, start, len);

    if(vma == NULL || vma->type != MMAP)
    {
        __debug_warn("[do_munmap] munmap address/sz illegal\n");
        return -1;
    }

    if(vma->fd == -1||(vma->mmap & MAP_PRIVATE))
    {
        goto ignore_wb;
    }

    struct file *f = (struct file *)MMAP_FILE(vma->mmap);
    if(f == NULL)
    {
        //__debug_warn("[do_munmap] open file not found\n");
        // return -1;
        goto ignore_wb;
    }

    uint64 pa, size, total_size = len;
    uint64 va = start;
    int page_n = (vma->end - vma->addr) >> PGSHIFT;

    f->off = vma->f_off;

    for(int i = 0; i < page_n; ++i)
    {
        size = total_size >= PGSIZE ? PGSIZE : total_size;
        pte_t * pte = walk(p->pagetable, va, 0);

        if((*pte & PTE_V) == 0)
        {
            __debug_warn("[do_munmap] page not valid\n");
            return -1;
        }
        if((*pte & PTE_U) == 0)
        {
            __debug_warn("[do_munmap] page is not user mode\n");
        }
        if(*pte & PTE_D){       // write back when PTE_D bit is 1
            pa = PTE2PA(*pte);
            if(!pa){
                __debug_warn("[do_munmap] page' pa is 0\n");
                return -1;
            }
            filewrite(f, va, size);
        }
        va += PGSIZE;
        total_size -= size;
    }
ignore_wb:
    if(!mf && free_vma(p, vma) == 0)
    {
        __debug_warn("[do_munmap] free vma failed\n");
        return -1;
    }else if(mf){
        kfree(mf);
    }

    return 0;

}

void
free_map_fix(struct proc* p){
    map_fix* mf = p->mf;
	map_fix* i = mf;
    map_fix* next;
    while(i){
        next = i->next;
        do_munmap(p,i->addr,i->sz);
        i = next;
    }
}


int do_msync(uint64 addr, uint64 len, int flags)
{
    struct proc *p = myproc();
    struct vma *s = part_locate_vma(p->vma, addr, addr + len);
    
    if(s == NULL)
    {
        return -EFAULT;
    }

    // private mapping and anonymous mapping can not be synchronized
    if(s->type != MMAP || MMAP_ANONY(s->mmap) || !MMAP_SHARE(s->mmap))
    {
        return 0;
    }

    // MS_INVALIDATE does nothing on our system
    // if (!(flags & (MS_ASYNC | MS_SYNC))) {
	// 	__debug_info("[do_msync] MS_INVALIDATE does nothing on our system\n");
	// 	return -1;
	// }

    // print_vma_info(p);

    struct file *f = MMAP_FILE(s->mmap);
    uint64 pa, size, total_size = len;
    uint64 va = addr;
    int page_n = (s->end - s->addr) >> PGSHIFT;

    f->off = s->f_off;


    for(int i = 0; i < page_n; ++i)
    {
        size = total_size >= PGSIZE ? PGSIZE : total_size;
        pte_t * pte = walk(p->pagetable, va, 0);

        if((*pte & PTE_V) == 0)
        {
            __debug_warn("[do_msync] page not valid\n");
            return -1;
        }
        if((*pte & PTE_U) == 0)
        {
            __debug_warn("[do_msync] page is not user mode\n");
        }
        if(*pte & PTE_D){       // write back when PTE_D bit is 1
            pa = PTE2PA(*pte);
            if(!pa){
                __debug_warn("[do_msync] page' pa is 0\n");
                return -1;
            }
            filewrite(f, va, size);
        }
        va += PGSIZE;
        total_size -= size;
    }

    return 0;
}
