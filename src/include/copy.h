#ifndef __COPY_H
#define __COPY_H

#include "types.h"
#include "riscv.h"
int             zero_out(uint64 dstva, uint64 len);
int             copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(int user_src, void *dst, uint64 src, uint64 len);


#endif
