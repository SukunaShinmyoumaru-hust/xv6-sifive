#include "include/timer.h"
#include "include/types.h"
#include "include/param.h"
#include "include/riscv.h"
#include "include/syscall.h"
#include "include/copy.h"
#include "include/file.h"
#include "include/errno.h"
#include "include/sysinfo.h"
#include "include/pm.h"
#include "include/poll.h"
#include "include/signal.h"

uint64
sys_ppoll(void){
  uint64 fds_addr;
  struct pollfd* fds;
  int nfds; 
  uint64 tm_addr;
  struct timespec tmo_p;
  uint64 sm_addr;
  __sigset_t sigmask;
  
  if(argint(1,&nfds) < 0)
  {
    return -1;
  }
  
  fds = (struct pollfd *)kmalloc(sizeof(struct pollfd) * nfds);
  
  if((fds_addr = argstruct(0, fds, nfds * sizeof(struct pollfd))) == NULL)
  {
    return -1;
  }
  
  tm_addr = argstruct(2, &tmo_p, sizeof(struct timespec));
  sm_addr = argstruct(3, &sigmask, sizeof(__sigset_t));
  
  uint64 ret = ppoll(fds, nfds,
  			tm_addr?&tmo_p:NULL, 
  			sm_addr?&sigmask:NULL);
  
  if(either_copyout(1, fds_addr, fds, sizeof(struct pollfd) * nfds) < 0)
  {
    return -1;
  }
  
  return ret;
}

uint64
sys_pselect6()
{
  
  return 0;
}

