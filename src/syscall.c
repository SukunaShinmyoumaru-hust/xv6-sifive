#include "include/syscall.h"

extern uint64 sys_dup(void);
extern uint64 sys_dup3(void);
extern uint64 sys_openat(void);
extern uint64 sys_read(void);
extern uint64 sys_write(void);
extern uint64 sys_exit(void);
extern uint64 sys_rt_sigaction(void);
extern uint64 sys_rt_sigreturn(void);
extern uint64 sys_getpid(void);
extern uint64 sys_getppid(void);
extern uint64 sys_execve(void);
static uint64 (*syscalls[])(void) = {
	[SYS_dup]	sys_dup,
	[SYS_dup3]	sys_dup3,
	[SYS_openat]	sys_openat,
	[SYS_read]	sys_read,
	[SYS_write]	sys_write,
	[SYS_exit]	sys_exit,
	[SYS_rt_sigaction]	sys_rt_sigaction,
	[SYS_rt_sigreturn]	sys_rt_sigreturn,
	[SYS_getpid]	sys_getpid,
	[SYS_getppid]	sys_getppid,
	[SYS_execve]	sys_execve,
};
static char *sysnames[] = {
	[SYS_dup]	"dup",
	[SYS_dup3]	"dup3",
	[SYS_openat]	"openat",
	[SYS_read]	"read",
	[SYS_write]	"write",
	[SYS_exit]	"exit",
	[SYS_rt_sigaction]	"rt_sigaction",
	[SYS_rt_sigreturn]	"rt_sigreturn",
	[SYS_getpid]	"getpid",
	[SYS_getppid]	"getppid",
	[SYS_execve]	"execve",
};
void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    //printf("syscall %d\n",num);
    p->trapframe->a0 = syscalls[num]();
        // trace
    if ((p->tmask & (1 << num)) != 0) {
      printf("pid %d: %s -> %d\n", p->pid, sysnames[num], p->trapframe->a0);
    }
  } else {
    printf("pid %d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
