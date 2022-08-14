#include "include/types.h"
#include "include/riscv.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/syscall.h"
#include "include/timer.h"
#include "include/kalloc.h"
#include "include/string.h"
#include "include/printf.h"
#include "include/console.h"
#include "include/string.h"
#include "include/vm.h"
#include "include/uname.h"
#include "include/copy.h"

uint64
sys_exit_group(void){
  //printf("exit group\n");
  return 0;
}

uint64
sys_rt_sigreturn(void){
  sigreturn();
  return 0;
}

uint64 sys_rt_sigprocmask(void){
  int how;
	uint64 uptr_set, uptr_oldset;

	__sigset_t set, oldset;

	argint(0, &how);
	argaddr(1, &uptr_set);
	argaddr(2, &uptr_oldset);

	if (uptr_set && either_copyin(1, (char*)&set, uptr_set, SIGSET_LEN * 8) < 0) {
		return -1;
	}

	if (sigprocmask(how, &set, uptr_oldset ? &oldset : NULL)) {
		return -1;
	}

	if (uptr_oldset && either_copyout(1, uptr_oldset, (char*)&oldset, SIGSET_LEN * 8) < 0) {
		return -1;
	}

	return 0;

}
uint64 sys_rt_sigaction(void) {
	int signum;
	uint64 uptr_act;		// struct sigaction const *act
	uint64 uptr_oldact;		// struct sigaction *oldact

	argint(0, &signum);
	argaddr(1, &uptr_act);
	argaddr(2, &uptr_oldact);

	// copy struct sigaction from user space 
	struct sigaction act;
	struct sigaction oldact;

	//__debug_info("[sigaction]  uptr_act : %p,uptr_oldact : %p\n",uptr_act,uptr_oldact);
	if (uptr_act) {
		if (
			either_copyin(1, (char*)&(act.__sigaction_handler), uptr_act, sizeof(__sighandler_t)) < 0 
		) {
			__debug_info("[sigaction] return -1\n");
			return -1;
		}
	}

	if (set_sigaction(
		signum, 
		uptr_act ? &act : NULL, 
		uptr_oldact ? &oldact : NULL
	) < 0) {
		//__debug_info("[sigaction] return -1\n");
		return -1;
	}

	if (uptr_oldact) {
		if (
			either_copyout(1,uptr_oldact, (char*)&(act.__sigaction_handler), sizeof(__sighandler_t)) < 0 
		) {
			//__debug_info("[sigaction] return -1\n");
			return -1;
		}
	}
	//__debug_info("[sigaction] return 0\n");
	return 0;
}

//int prlimit(pid_t pid, int resource, const struct rlimit *new_limit,struct rlimit *old_limit);
uint64
sys_prlimit64(void){
  //printf("prlimit64\n");
  int pid;
  int resource;
  uint64 newrlimitaddr;
  uint64 oldrlimitaddr;
  struct rlimit new_limit;
  struct rlimit old_limit;
  struct proc* p = myproc();
  
  if(argint(0,&pid)<0){
    return -1;
  }

  struct proc* limitp = pid==0?p:findproc(pid);
  if(!limitp)return -1;

  if(argint(1,&resource)<0){
    return -1;
  }
  newrlimitaddr = argstruct(2,&new_limit,sizeof(new_limit));
  oldrlimitaddr = argstruct(3,&old_limit,sizeof(old_limit));
  if(!newrlimitaddr && !oldrlimitaddr) return -1;
  
  // printf("[prlimit]pid:%d resource:%d\n",pid,resource);
  // if(newrlimitaddr)printf("[prlimit]new limit %d %d\n",new_limit.rlim_cur,new_limit.rlim_max);
  // else printf("[prlimit]new limit (nil)\n");
  // if(oldrlimitaddr)printf("[prlimit]old limit %d %d\n",old_limit.rlim_cur,old_limit.rlim_max);
  // else printf("[prlimit]old limit (nil)\n");
  
  switch(resource){
    case RLIMIT_NOFILE:
      if(oldrlimitaddr){
        old_limit.rlim_cur = old_limit.rlim_max = NOFILEMAX(limitp);
        if(copyout(p->pagetable,oldrlimitaddr,(char*)&old_limit,sizeof(old_limit))<0){
          // __debug_info("[sys_prlimit] copyout error\n");
          return -1;
        }
      }

      if(newrlimitaddr){
        limitp->filelimit = MIN(new_limit.rlim_cur,new_limit.rlim_max);
      }
      break;
    default:
    	return -1;
  }
  return 0;
}

uint64 sys_kill(){
  int sig;
  int pid;
  argint(0,&pid);
  argint(1,&sig);
  return kill(pid,sig);
}

uint64 sys_tgkill(){
  int sig;
  int tid;
  int pid;
  argint(0,&pid);
  argint(1,&tid);
  argint(2,&sig);
  return tgkill(pid,tid,sig);
}
