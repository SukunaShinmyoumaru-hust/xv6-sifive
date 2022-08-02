#ifndef __DISK_H
#define __DISK_H

#include "buf.h"

void disk_init(void);
void vdisk_read(struct buf *b);
void vdisk_write(struct buf *b);
void disk_intr(void);

#endif
