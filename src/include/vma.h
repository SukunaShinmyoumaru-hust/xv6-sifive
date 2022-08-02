#ifndef __VMA_H
#define __VMA_H

#include "types.h"
#include "file.h"
#define ALIGNUP(x,align) (-(-(x)) & (-(align)))
#define ALIGNDOWN(x,align) ((x) & (- (align)))
enum segtype {NONE, LOAD, TEXT, DATA, BSS, HEAP, MMAP, STACK,TAIL};

struct vma {
    enum segtype type;
    int flags;
    uint64 addr;
    uint64 sz;
    uint64 end;
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

struct vma* locatevma(struct vma *head, uint64 addr);
struct vma *type_getvma(struct vma *head, enum segtype type);
struct vma *partof_getvma(struct vma *head, uint64 start, uint64 end);
void delvmas(struct vma* head);
struct vma *copyvmas(struct vma* p);
struct vma *delvma(struct vma *head, uint64 start, uint64 sz);

#endif

