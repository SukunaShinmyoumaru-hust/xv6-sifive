#if !defined(RAMDISK_H)
#define RAMDISK_H

struct buf;

void ramdisk_init();
void ramdisk_rw(struct buf *b, int write);

#endif // RAMDISK_H
