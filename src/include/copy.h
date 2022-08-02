#ifndef __COPY_H
#define __COPY_H

#include "types.h"
int             zero_out(uint64 dstva, uint64 len);
int             either_copyout(int user_dst, uint64 dst, void *src, uint64 len);
int             either_copyin(void *dst, int user_src, uint64 src, uint64 len);


#endif
