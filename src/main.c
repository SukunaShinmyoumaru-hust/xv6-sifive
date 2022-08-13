// Copyright (c) 2006-2019 Frans Kaashoek, Robert Morris, Russ Cox,
//                         Massachusetts Institute of Technology

#include "include/types.h"
#include "include/param.h"
#include "include/riscv.h"
#include "include/sbi.h"
#include "include/cpu.h"
#include "include/pm.h"
#include "include/vm.h"
#include "include/kalloc.h"
#include "include/disk.h"
#include "include/timer.h"
#include "include/trap.h"
#include "include/plic.h"
#include "include/printf.h"
#include "include/proc.h"
#include "include/buf.h"
#include "include/dev.h"
#include "include/sysinfo.h"
static inline void inithartid(unsigned long hartid) {
  asm volatile("mv tp, %0" : : "r" (hartid));
}
int __get_boot_hartid(int a0);

volatile unsigned long __first_boot_magic = 0x5a5a;

volatile static int started = 0;
int booted[NCPU];
struct sbiret state[NCPU];
extern char _entry[];
/*
static inline void checkall(){
  for(int i = 1; i < NCPU; i++) {
	state[i]=get_state(i);
  }
  printf("%d %d\n%d %d\n%d %d\n%d %d\n",
  				  state[1].error,state[1].value,
  				  state[2].error,state[2].value,
  				  state[3].error,state[3].value,
  				  state[4].error,state[4].value);
}
*/

void
main(unsigned long hartid, unsigned long dtb_pa)
{
  inithartid(hartid);
  booted[hartid]=1;
  
  if (__first_boot_magic == 0x5a5a) { /* boot hart not fixed 1 */
    __first_boot_magic = 0;
    cpuinit();
    printfinit();
    printf("hart %d enter main() from %p...\n", hartid,_entry);
    for(int i = 1; i < NCPU; i++) {
        printf("cpu#%d state:%d\r\n", i, sbi_hsm_hart_status(i));
    }
    kpminit();
    kmallocinit();
    kvminit();       // create kernel page table
    kvminithart();   // turn on paging
    timerinit();     // init a lock for timer
    trapinithart();  // install kernel trap vector, including interrupt handler
    procinit();
    plicinit();
    binit();
    disk_init();
    fs_init();
    logbufinit();
    devinit();
    fileinit();
    
    //for(int j =0;j<68;j++){
      userinit();
    //}
    __sync_synchronize();

    for(int i = 1; i < NCPU; i++) {
        if(hartid!=i&&booted[i]==0){
          start_hart(i, (uint64)_entry, 0);
        }
    }
    started=1;
  }
  else
  {
    // hart 1
    while (started == 0)
    ;
    printf("hart %d enter main()...\n", hartid);
    kvminithart();
    trapinithart();  // install kernel trap vector, including interrupt handler
    __sync_synchronize();
  }
  printf("hart %d scheduler!\n", hartid);
  scheduler();
}

int
__get_boot_hartid(int a0)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        if (sbi_hsm_hart_status(i) == 0)
        {
            return i;
        }
    }
    return a0;
}
