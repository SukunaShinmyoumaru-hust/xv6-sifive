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
  	int nfds;
	uint64 urfds, uwfds, uexfds;
	uint64 utoaddr, usmaddr;
	
	argint(0, &nfds);
	argaddr(1, &urfds);
	argaddr(2, &uwfds);
	argaddr(3, &uexfds);
	argaddr(4, &utoaddr);
	argaddr(5, &usmaddr);
	
	if (nfds <= 0 || nfds > FDSET_SIZE)
		return -EINVAL;
	if (!(urfds || uwfds || uexfds))
		return -EINVAL;

	struct fdset rfds, wfds, exfds;
	struct timespec timeout;
	__sigset_t sigmask;

	if (urfds && either_copyin(1, (char *)&rfds, urfds, sizeof(struct fdset)) < 0)
		return -EFAULT;
	if (uwfds && either_copyin(1, (char *)&wfds, uwfds, sizeof(struct fdset)) < 0)
		return -EFAULT;
	if (uexfds && either_copyin(1, (char *)&exfds, uexfds, sizeof(struct fdset)) < 0)
		return -EFAULT;
	if (utoaddr && either_copyin(1, (char *)&timeout, utoaddr, sizeof(timeout)) < 0)
		return -EFAULT;
	if (usmaddr && either_copyin(1, (char *)&sigmask, usmaddr, sizeof(sigmask)) < 0)
		return -EFAULT;

	struct proc *p = myproc();
	if (p->tmask) {
		printf(") ...\n");
	}

	int ret = pselect(nfds,
				urfds ? &rfds: NULL,
				uwfds ? &wfds: NULL,
				uexfds ? &exfds: NULL,
				utoaddr ? &timeout : NULL,
				usmaddr ? &sigmask : NULL
			);

	if (urfds)
		either_copyout(1, urfds, (char *)&rfds, sizeof(struct fdset));
	if (uwfds)
		either_copyout(1, uwfds, (char *)&wfds, sizeof(struct fdset));
	if (uexfds)
		either_copyout(1, uexfds, (char *)&exfds, sizeof(struct fdset));

	if (p->tmask) {
		printf("pid %d: return from pselect(", p->pid);
	}

	//__debug_info("[sys_pselect6] ret = %d\n", ret);
	return ret;
}

