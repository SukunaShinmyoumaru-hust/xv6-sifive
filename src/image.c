#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/printf.h"
#include "include/buf.h"
#include "include/image.h"
#include "include/diskio.h"
#include "include/ramdisk.h"


#ifdef SIFIVE_U
#include "include/diskio.h"
#else
#include "include/ramdisk.h"
#endif

void image_init(struct dirent* img)
{
    if(!FatFs[img->dev].valid){
      panic("no support filesystem");
    }
    return;
}

void image_read(struct buf *b,struct dirent* img)
{
  uint sectorno = b->sectorno;
  int off = sectorno*BSIZE;
  elock(img);
  if(eread(img,0,(uint64)(b->data),off,BSIZE)<0)panic("read image error");
  eunlock(img);
  return;
}

void image_write(struct buf *b,struct dirent* img)
{
  uint sectorno = b->sectorno;
  int off = sectorno*BSIZE;
  elock(img);
  if(ewrite(img,0,(uint64)(b->data),off,BSIZE)<0)panic("write image error");
  eunlock(img);
  return;
}

