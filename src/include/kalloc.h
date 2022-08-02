#ifndef __KALLOC_H
#define __KALLOC_H

#include "types.h"

void 			kmallocinit(void);
/* 
 * allocate a range of mem of wanted size 
 */
void*           kmalloc(uint size);

/* 
 * free memory starts with `addr`
 */
void            kfree(void *addr);


#endif
