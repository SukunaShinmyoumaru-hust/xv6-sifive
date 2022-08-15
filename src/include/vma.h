#ifndef __VMA_H
#define __VMA_H

#include "types.h"
#include "utils/list.h"
#include "proc.h"
#include "mmap.h"

#define ALIGNUP(x,align) (-(-(x)) & (-(align)))
#define ALIGNDOWN(x,align) ((x) & (- (align)))
enum segtype {NONE, LOAD, TEXT, DATA, BSS, HEAP, MMAP, STACK, TRAP};


struct vma {
    enum segtype type;
    int perm;
    uint64 addr;
    uint64 sz;
    uint64 end;
    uint64 mmap;
    int flags;
    int fd;
    uint64 f_off;
    struct vma *prev;
    struct vma *next;
};

struct vma *vma_list_init(struct proc *p);
struct vma *alloc_vma(struct proc *p, enum segtype type, uint64 addr, uint64 sz, int perm, int alloc, uint64 pa);
struct vma* type_locate_vma(struct vma *head, enum segtype type);
struct vma *addr_locate_vma(struct vma*head, uint64 addr);
struct vma *addr_sz_locate_vma(struct vma*head, uint64 addr, uint64 sz);
struct vma *part_locate_vma(struct vma *head, uint64 start, uint64 end);
struct vma *alloc_mmap_vma(struct proc *p, int flags, uint64 addr, uint64 sz, int perm, int fd ,uint64 f_off);
struct vma *alloc_stack_vma(struct proc *p, uint64 addr, int perm);
struct vma *alloc_addr_heap_vma(struct proc *p, uint64 addr, int perm);
struct vma *alloc_sz_heap_vma(struct proc *p, uint64 sz, int perm);
struct vma *alloc_load_vma(struct proc *p, uint64 addr, uint64 sz, int perm);
struct vma *vma_copy(struct proc *np, struct vma *head);
int free_vma_list(struct proc *p);
int free_vma(struct proc *p, struct vma *del);
uint64 growproc(int n);
uint64 growprocsize(uint64 sz);
void print_vma_info(struct proc* p);
void print_single_vma(pagetable_t pagetable,struct vma* v);
int vma_deep_mapping(pagetable_t old, pagetable_t new, const struct vma *vma);
int vma_shallow_mapping(pagetable_t old, pagetable_t new, const struct vma *vma);
#endif

