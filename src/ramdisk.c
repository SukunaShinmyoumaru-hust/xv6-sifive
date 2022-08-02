#include "include/types.h"
#include "include/riscv.h"
#include "include/defs.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/printf.h"
#include "include/spinlock.h"
#include "include/buf.h"

#define USE_RAMDISK
#define NRAMDISKPAGES (FSSIZE * BSIZE / PGSIZE)

struct spinlock ramdisklock;
extern char fs_img_start[];
extern char fs_img_end[];
char* ramdisk;

void
ramdisk_init(void)
{
#ifdef QEMU
  ramdisk = fs_img_start;
#endif
#ifdef SIFIVE_U
  ramdisk = (char*)RAMDISK;
#endif
  initlock(&ramdisklock, "ramdisk lock");
  __debug_info("ramdiskinit ram start:%p\n",ramdisk);
}

void 
ramdisk_rw(struct buf *b, int write)
{
  acquire(&ramdisklock);
  uint sectorno = b->sectorno;

  char *addr = ramdisk + sectorno * BSIZE;
  if (write)
  {
    memmove((void*)addr, b->data, BSIZE);
  }
  else
  {
    memmove(b->data, (void*)addr, BSIZE);
  }
  release(&ramdisklock);
}

void
ramdisk_intr()
{
    //acquire(&ramdisklock);
    
}
