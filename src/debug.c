#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/vm.h"
#include "include/debug.h"
#include "include/trap.h"
#include "include/syscall.h"
#include "include/printf.h"
#include "include/copy.h"


int
debug7c890(struct proc* p,uint64 va){
  //pagetable_t pagetable = p->pagetable;
  char buf[20];
  int n = p->trapframe->a2;
  if(either_copyin(1,buf,p->trapframe->a1,n)<0){
    printf("???\n");
  }else{
    printf("buf:%s\n",buf);
  }
  p->trapframe->sp-=64;
  p->trapframe->epc+=2;
  return 0;
}

int
debugaddr()
{
  struct proc* p = myproc();
  uint64 va = p->trapframe->epc;
  if(va==0x7c890){
    return debug7c890(p,va);
  }
  /*
  if(va == 0xf52f78){
    return debugf52f78(p,va);
  }else if(va == 0xf50e08){
    return debugf50e08(p,va);
  }else if(va == 0xf50d8e){
    return debugf50d8e(p,va);
  }else if(va == 0xf524b2){
    return debugf524b2(p,va);
  }else if(va == 0xf51d7e){
    return debugf51d7e(p,va);
  }
*/
  return -1;
}
