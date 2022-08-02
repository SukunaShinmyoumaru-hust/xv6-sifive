#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/printf.h"
#include "include/buf.h"


#ifdef SD
#include "include/diskio.h"
#else
#include "include/ramdisk.h"
#endif

int disk_init_flag;
void disk_init(void)
{
    if(disk_init_flag)return;
    else disk_init_flag = 1;
    #ifdef RAM
    ramdisk_init();
    #else
    disk_initialize(0);
    #endif
}

void vdisk_read(struct buf *b)
{
    #ifdef RAM    
	ramdisk_rw(b, 0);
    #else 
	disk_read(0,b->data, b->sectorno,1);
    #endif
}

void vdisk_write(struct buf *b)
{
    #ifdef RAM
    	ramdisk_rw(b, 1);
    #else 
	disk_write(0,b->data, b->sectorno,1);
    #endif
}

void disk_intr(void)
{
    #ifdef SD
        // dmac_intr(DMAC_CHANNEL0);
    #endif
}
