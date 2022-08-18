#include "include/syscall.h"

extern uint64 sys_getcwd(void);
extern uint64 sys_epoll_create1(void);
extern uint64 sys_epoll_ctl(void);
extern uint64 sys_epoll_pwait(void);
extern uint64 sys_dup(void);
extern uint64 sys_dup3(void);
extern uint64 sys_fcntl(void);
extern uint64 sys_ioctl(void);
extern uint64 sys_mkdirat(void);
extern uint64 sys_unlinkat(void);
extern uint64 sys_faccessat(void);
extern uint64 sys_openat(void);
extern uint64 sys_close(void);
extern uint64 sys_pipe2(void);
extern uint64 sys_getdents64(void);
extern uint64 sys_lseek(void);
extern uint64 sys_read(void);
extern uint64 sys_write(void);
extern uint64 sys_readv(void);
extern uint64 sys_writev(void);
extern uint64 sys_sendfile(void);
extern uint64 sys_pselect6(void);
extern uint64 sys_ppoll(void);
extern uint64 sys_readlinkat(void);
extern uint64 sys_fstatat(void);
extern uint64 sys_fstat(void);
extern uint64 sys_fsync(void);
extern uint64 sys_utimensat(void);
extern uint64 sys_exit(void);
extern uint64 sys_exit_group(void);
extern uint64 sys_set_tid_address(void);
extern uint64 sys_futex(void);
extern uint64 sys_nanosleep(void);
extern uint64 sys_setitimer(void);
extern uint64 sys_clock_gettime(void);
extern uint64 sys_syslog(void);
extern uint64 sys_kill(void);
extern uint64 sys_tgkill(void);
extern uint64 sys_rt_sigaction(void);
extern uint64 sys_rt_sigprocmask(void);
extern uint64 sys_rt_sigreturn(void);
extern uint64 sys_setgid(void);
extern uint64 sys_setuid(void);
extern uint64 sys_uname(void);
extern uint64 sys_getrusage(void);
extern uint64 sys_umask(void);
extern uint64 sys_prctl(void);
extern uint64 sys_getpid(void);
extern uint64 sys_getppid(void);
extern uint64 sys_getuid(void);
extern uint64 sys_geteuid(void);
extern uint64 sys_getgid(void);
extern uint64 sys_getegid(void);
extern uint64 sys_gettid(void);
extern uint64 sys_sysinfo(void);
extern uint64 sys_socket(void);
extern uint64 sys_bind(void);
extern uint64 sys_listen(void);
extern uint64 sys_connect(void);
extern uint64 sys_sendto(void);
extern uint64 sys_recvfrom(void);
extern uint64 sys_setsockopt(void);
extern uint64 sys_brk(void);
extern uint64 sys_munmap(void);
extern uint64 sys_clone(void);
extern uint64 sys_execve(void);
extern uint64 sys_mmap(void);
extern uint64 sys_mprotect(void);
extern uint64 sys_msync(void);
extern uint64 sys_wait4(void);
extern uint64 sys_prlimit64(void);
extern uint64 sys_renameat2(void);
static uint64 (*syscalls[])(void) = {
	[SYS_getcwd]	sys_getcwd,
	[SYS_epoll_create1]	sys_epoll_create1,
	[SYS_epoll_ctl]	sys_epoll_ctl,
	[SYS_epoll_pwait]	sys_epoll_pwait,
	[SYS_dup]	sys_dup,
	[SYS_dup3]	sys_dup3,
	[SYS_fcntl]	sys_fcntl,
	[SYS_ioctl]	sys_ioctl,
	[SYS_mkdirat]	sys_mkdirat,
	[SYS_unlinkat]	sys_unlinkat,
	[SYS_faccessat]	sys_faccessat,
	[SYS_openat]	sys_openat,
	[SYS_close]	sys_close,
	[SYS_pipe2]	sys_pipe2,
	[SYS_getdents64]	sys_getdents64,
	[SYS_lseek]	sys_lseek,
	[SYS_read]	sys_read,
	[SYS_write]	sys_write,
	[SYS_readv]	sys_readv,
	[SYS_writev]	sys_writev,
	[SYS_sendfile]	sys_sendfile,
	[SYS_pselect6]	sys_pselect6,
	[SYS_ppoll]	sys_ppoll,
	[SYS_readlinkat]	sys_readlinkat,
	[SYS_fstatat]	sys_fstatat,
	[SYS_fstat]	sys_fstat,
	[SYS_fsync]	sys_fsync,
	[SYS_utimensat]	sys_utimensat,
	[SYS_exit]	sys_exit,
	[SYS_exit_group]	sys_exit_group,
	[SYS_set_tid_address]	sys_set_tid_address,
	[SYS_futex]	sys_futex,
	[SYS_nanosleep]	sys_nanosleep,
	[SYS_setitimer]	sys_setitimer,
	[SYS_clock_gettime]	sys_clock_gettime,
	[SYS_syslog]	sys_syslog,
	[SYS_kill]	sys_kill,
	[SYS_tgkill]	sys_tgkill,
	[SYS_rt_sigaction]	sys_rt_sigaction,
	[SYS_rt_sigprocmask]	sys_rt_sigprocmask,
	[SYS_rt_sigreturn]	sys_rt_sigreturn,
	[SYS_setgid]	sys_setgid,
	[SYS_setuid]	sys_setuid,
	[SYS_uname]	sys_uname,
	[SYS_getrusage]	sys_getrusage,
	[SYS_umask]	sys_umask,
	[SYS_prctl]	sys_prctl,
	[SYS_getpid]	sys_getpid,
	[SYS_getppid]	sys_getppid,
	[SYS_getuid]	sys_getuid,
	[SYS_geteuid]	sys_geteuid,
	[SYS_getgid]	sys_getgid,
	[SYS_getegid]	sys_getegid,
	[SYS_gettid]	sys_gettid,
	[SYS_sysinfo]	sys_sysinfo,
	[SYS_socket]	sys_socket,
	[SYS_bind]	sys_bind,
	[SYS_listen]	sys_listen,
	[SYS_connect]	sys_connect,
	[SYS_sendto]	sys_sendto,
	[SYS_recvfrom]	sys_recvfrom,
	[SYS_setsockopt]	sys_setsockopt,
	[SYS_brk]	sys_brk,
	[SYS_munmap]	sys_munmap,
	[SYS_clone]	sys_clone,
	[SYS_execve]	sys_execve,
	[SYS_mmap]	sys_mmap,
	[SYS_mprotect]	sys_mprotect,
	[SYS_msync]	sys_msync,
	[SYS_wait4]	sys_wait4,
	[SYS_prlimit64]	sys_prlimit64,
	[SYS_renameat2]	sys_renameat2,
};
static char *sysnames[] = {
	[SYS_getcwd]	"getcwd",
	[SYS_epoll_create1]	"epoll_create1",
	[SYS_epoll_ctl]	"epoll_ctl",
	[SYS_epoll_pwait]	"epoll_pwait",
	[SYS_dup]	"dup",
	[SYS_dup3]	"dup3",
	[SYS_fcntl]	"fcntl",
	[SYS_ioctl]	"ioctl",
	[SYS_mkdirat]	"mkdirat",
	[SYS_unlinkat]	"unlinkat",
	[SYS_faccessat]	"faccessat",
	[SYS_openat]	"openat",
	[SYS_close]	"close",
	[SYS_pipe2]	"pipe2",
	[SYS_getdents64]	"getdents64",
	[SYS_lseek]	"lseek",
	[SYS_read]	"read",
	[SYS_write]	"write",
	[SYS_readv]	"readv",
	[SYS_writev]	"writev",
	[SYS_sendfile]	"sendfile",
	[SYS_pselect6]	"pselect6",
	[SYS_ppoll]	"ppoll",
	[SYS_readlinkat]	"readlinkat",
	[SYS_fstatat]	"fstatat",
	[SYS_fstat]	"fstat",
	[SYS_fsync]	"fsync",
	[SYS_utimensat]	"utimensat",
	[SYS_exit]	"exit",
	[SYS_exit_group]	"exit_group",
	[SYS_set_tid_address]	"set_tid_address",
	[SYS_futex]	"futex",
	[SYS_nanosleep]	"nanosleep",
	[SYS_setitimer]	"setitimer",
	[SYS_clock_gettime]	"clock_gettime",
	[SYS_syslog]	"syslog",
	[SYS_kill]	"kill",
	[SYS_tgkill]	"tgkill",
	[SYS_rt_sigaction]	"rt_sigaction",
	[SYS_rt_sigprocmask]	"rt_sigprocmask",
	[SYS_rt_sigreturn]	"rt_sigreturn",
	[SYS_setgid]	"setgid",
	[SYS_setuid]	"setuid",
	[SYS_uname]	"uname",
	[SYS_getrusage]	"getrusage",
	[SYS_umask]	"umask",
	[SYS_prctl]	"prctl",
	[SYS_getpid]	"getpid",
	[SYS_getppid]	"getppid",
	[SYS_getuid]	"getuid",
	[SYS_geteuid]	"geteuid",
	[SYS_getgid]	"getgid",
	[SYS_getegid]	"getegid",
	[SYS_gettid]	"gettid",
	[SYS_sysinfo]	"sysinfo",
	[SYS_socket]	"socket",
	[SYS_bind]	"bind",
	[SYS_listen]	"listen",
	[SYS_connect]	"connect",
	[SYS_sendto]	"sendto",
	[SYS_recvfrom]	"recvfrom",
	[SYS_setsockopt]	"setsockopt",
	[SYS_brk]	"brk",
	[SYS_munmap]	"munmap",
	[SYS_clone]	"clone",
	[SYS_execve]	"execve",
	[SYS_mmap]	"mmap",
	[SYS_mprotect]	"mprotect",
	[SYS_msync]	"msync",
	[SYS_wait4]	"wait4",
	[SYS_prlimit64]	"prlimit64",
	[SYS_renameat2]	"renameat2",
};
void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    //printf("[start]-----pid:%d syscall %d:%s-----\n",myproc()->pid,num,sysnames[num]);
    p->trapframe->a0 = syscalls[num]();
    //printf("[end]----pid:%d syscall %d:%s return %p-----\n",myproc()->pid,num,sysnames[num],p->trapframe->a0);
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
