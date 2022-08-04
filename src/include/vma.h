#ifndef __VMA_H
#define __VMA_H

#include "types.h"
#include "file.h"
#include "utils/list.h"
#include "proc.h"

#define ALIGNUP(x,align) (-(-(x)) & (-(align)))
#define ALIGNDOWN(x,align) ((x) & (- (align)))
enum segtype {NONE, LOAD, TEXT, DATA, BSS, HEAP, MMAP, STACK, TRAP};


struct vma {
    enum segtype type;
    int perm;
    uint64 addr;
    uint64 sz;
    uint64 end;
    struct file *f;
    uint64 f_off;
    struct vma *prev;
    struct vma *next;
};

struct vma *newvma(
    struct vma* head,
    enum segtype type,
    uint64 offset,
    uint64 sz,
    long flag,
    struct file *f
);

struct vma *vma_list_init(struct proc *p);
struct vma *alloc_vma(struct proc *p, enum segtype type, uint64 addr, uint64 sz, int perm, int alloc, uint64 pa);
struct vma* type_locate_vma(struct vma *head, enum segtype type);
struct vma *addr_locate_vma(struct vma*head, uint64 addr);
struct vma* alloc_mmap_vma(struct proc *p, uint64 addr, uint64 sz, int perm, struct file *f ,uint64 f_off);
struct vma *alloc_stack_vma(struct proc *p, uint64 addr, int perm);
struct vma *alloc_addr_heap_vma(struct proc *p, uint64 addr, int perm);
struct vma *alloc_sz_heap_vma(struct proc *p, uint64 sz, int perm);
struct vma *alloc_load_vma(struct proc *p, uint64 addr, uint64 sz, int perm);
int free_vma_list(struct proc *p);
int free_vma(struct proc *p, uint64 addr, uint64 len);
void print_vma_info(struct vma *head);
#endif

