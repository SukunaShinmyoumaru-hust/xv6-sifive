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

uint64
sys_exit_group(void){
  //printf("exit group\n");
  return 0;
}

uint64 sys_rt_sigaction(void) {
	int signum;
	uint64 uptr_act;		// struct sigaction const *act
	uint64 uptr_oldact;		// struct sigaction *oldact
	int size;

	// extract user args
	// if (argint(0, &signum) < 0) {
	// 	return -EINVAL;
	// }
	// if (argaddr(1, &uptr_act) < 0) {
	// 	return -EINVAL;
	// }
	// if (argaddr(2, &uptr_oldact) < 0) {
	// 	return -EINVAL;
	// }
	argint(0, &signum);
	argaddr(1, &uptr_act);
	argaddr(2, &uptr_oldact);
	argint(3, &size);

	// copy struct sigaction from user space 
	struct sigaction act;
	struct sigaction oldact;
	// if (uptr_act && copyin2((char*)&act, uptr_act, sizeof(struct sigaction)) < 0) {
	// 	return -EFAULT;
	// }
	if (size > 16 * 8) {
		size = 16 * 8;
	}
	else if (size < 8) {
		size = 8;
	}

	if (uptr_act) {
		if (
			copyin2((char*)&(act.__sigaction_handler), uptr_act, sizeof(__sighandler_t)) < 0 
			// copyin2((char*)&(act.sa_mask), uptr_act + sizeof(__sighandler_t), size) < 0 || 
			// copyin2((char*)&(act.sa_flags), uptr_act + sizeof(__sighandler_t) + size, sizeof(int)) < 0
		) {
			return -1;
		}
	}

	if (set_sigaction(
		signum, 
		uptr_act ? &act : NULL, 
		uptr_oldact ? &oldact : NULL
	) < 0) {
		return -1;
	}

	// copyout old struct sigaction 
	// if (uptr_oldact && copyout2(uptr_oldact, (char*)&oldact, sizeof(struct sigaction)) < 0) {
	// 	return -EFAULT;
	// }
	if (uptr_oldact) {
		if (
			copyout2(uptr_oldact, (char*)&(act.__sigaction_handler), sizeof(__sighandler_t)) < 0 
			// copyout2(uptr_oldact + sizeof(__sighandler_t), (char*)&(act.sa_mask), size) < 0 || 
			// copyout2(uptr_oldact + sizeof(__sighandler_t) + size, (char*)&(act.sa_flags), sizeof(int)) < 0
		) {
			return -1;
		}
	}

	return 0;
}
//extern int proc_mask_times;
uint64 sys_rt_sigprocmask(void) {
	int how;
	uint64 uptr_set, uptr_oldset;

	__sigset_t set, oldset;
	int size;

	argint(0, &how);
	argaddr(1, &uptr_set);
	argaddr(2, &uptr_oldset);
	argint(3, &size);

	if (size > 16 * 8) {
		size = 16 * 8;
	}
	else if (size < 8) {
		size = 8;
	}
  // printf("[sigprocmask]set:%p\n",set.__val[0]);
	// copy in __sigset_t 
	if (uptr_set && copyin2((char*)&set, uptr_set, size) < 0) {
		return -1;
	}

	if (sigprocmask(
		how, 
		&set, 
		uptr_oldset ? &oldset : NULL
	)) {
		return -1;
	}
  //proc_mask_times++;
  //if((proc_mask_times == 8))oldset.__val[0] = 0xffffffff;
  // printf("proc_mask_times:%d\n",proc_mask_times);
	if (uptr_oldset && copyout2(uptr_oldset, (char*)&oldset, size) < 0) {
		return -1;
	}

	return 0;
}


uint64
sys_rt_sigreturn(void){
  sigreturn();
  return 0;
}
