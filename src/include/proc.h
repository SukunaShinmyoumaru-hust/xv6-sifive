#ifndef __PROC_H
#define __PROC_H

#include "param.h"
#include "riscv.h"
#include "types.h"
#include "spinlock.h"
#include "file.h"
#include "fat32.h"
#include "trap.h"
#include "signal.h"
#include "timer.h"
#include "cpu.h"
#include "utils/list.h"
#include "vma.h"
#include "mmap.h"

#define FUTEX_WAIT		0
#define FUTEX_WAKE		1
#define FUTEX_FD		2
#define FUTEX_REQUEUE		3
#define FUTEX_CMP_REQUEUE	4
#define FUTEX_WAKE_OP		5
#define FUTEX_LOCK_PI		6
#define FUTEX_UNLOCK_PI		7
#define FUTEX_TRYLOCK_PI	8
#define FUTEX_WAIT_BITSET	9
#define FUTEX_WAKE_BITSET	10
#define FUTEX_WAIT_REQUEUE_PI	11
#define FUTEX_CMP_REQUEUE_PI	12
#define FUTEX_LOCK_PI2		13

#define FUTEX_PRIVATE_FLAG	128
#define FUTEX_CLOCK_REALTIME	256
#define FUTEX_CMD_MASK		~(FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME)

#define FUTEX_WAIT_PRIVATE	(FUTEX_WAIT | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_PRIVATE	(FUTEX_WAKE | FUTEX_PRIVATE_FLAG)
#define FUTEX_REQUEUE_PRIVATE	(FUTEX_REQUEUE | FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PRIVATE (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_OP_PRIVATE	(FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG)
#define FUTEX_LOCK_PI_PRIVATE	(FUTEX_LOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_LOCK_PI2_PRIVATE	(FUTEX_LOCK_PI2 | FUTEX_PRIVATE_FLAG)
#define FUTEX_UNLOCK_PI_PRIVATE	(FUTEX_UNLOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_TRYLOCK_PI_PRIVATE (FUTEX_TRYLOCK_PI | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAIT_BITSET_PRIVATE	(FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAKE_BITSET_PRIVATE	(FUTEX_WAKE_BITSET | FUTEX_PRIVATE_FLAG)
#define FUTEX_WAIT_REQUEUE_PI_PRIVATE	(FUTEX_WAIT_REQUEUE_PI | \
					 FUTEX_PRIVATE_FLAG)
#define FUTEX_CMP_REQUEUE_PI_PRIVATE	(FUTEX_CMP_REQUEUE_PI | \
					 FUTEX_PRIVATE_FLAG)

#define FLAGS_SHARED		0x00
#define FLAGS_CLOCKRT		0x02
#define FLAGS_HAS_TIMEOUT	0x04
#define FUTEX_BITSET_MATCH_ANY 0xffffffff


/*
 * cloning flags:
 */
#define CSIGNAL		0x000000ff	/* signal mask to be sent at exit */
#define CLONE_VM	0x00000100	/* set if VM shared between processes */
#define CLONE_FS	0x00000200	/* set if fs info shared between processes */
#define CLONE_FILES	0x00000400	/* set if open files shared between processes */
#define CLONE_SIGHAND	0x00000800	/* set if signal handlers and blocked signals shared */
#define CLONE_PIDFD	0x00001000	/* set if a pidfd should be placed in parent */
#define CLONE_PTRACE	0x00002000	/* set if we want to let tracing continue on the child too */
#define CLONE_VFORK	0x00004000	/* set if the parent wants the child to wake it up on mm_release */
#define CLONE_PARENT	0x00008000	/* set if we want to have the same parent as the cloner */
#define CLONE_THREAD	0x00010000	/* Same thread group? */
#define CLONE_NEWNS	0x00020000	/* New mount namespace group */
#define CLONE_SYSVSEM	0x00040000	/* share system V SEM_UNDO semantics */
#define CLONE_SETTLS	0x00080000	/* create a new TLS for the child */
#define CLONE_PARENT_SETTID	0x00100000	/* set the TID in the parent */
#define CLONE_CHILD_CLEARTID	0x00200000	/* clear the TID in the child */
#define CLONE_DETACHED		0x00400000	/* Unused, ignored */
#define CLONE_UNTRACED		0x00800000	/* set if the tracing process can't force CLONE_PTRACE on this clone */
#define CLONE_CHILD_SETTID	0x01000000	/* set the TID in the child */
#define CLONE_NEWCGROUP		0x02000000	/* New cgroup namespace */
#define CLONE_NEWUTS		0x04000000	/* New utsname namespace */
#define CLONE_NEWIPC		0x08000000	/* New ipc namespace */
#define CLONE_NEWUSER		0x10000000	/* New user namespace */
#define CLONE_NEWPID		0x20000000	/* New pid namespace */
#define CLONE_NEWNET		0x40000000	/* New network namespace */
#define CLONE_IO		0x80000000	/* Clone io context */


enum procstate { UNUSED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

#define RLIMIT_CPU     0
#define RLIMIT_FSIZE   1
#define RLIMIT_DATA    2
#define RLIMIT_STACK   3
#define RLIMIT_CORE    4
#define RLIMIT_RSS     5
#define RLIMIT_NPROC   6
#define RLIMIT_NOFILE  7
#define RLIMIT_MEMLOCK 8
#define RLIMIT_AS      9
#define RLIMIT_LOCKS   10
#define RLIMIT_SIGPENDING 11
#define RLIMIT_MSGQUEUE 12
#define RLIMIT_NICE    13
#define RLIMIT_RTPRIO  14
#define RLIMIT_RTTIME  15
#define RLIMIT_NLIMITS 16

#define PROCMAGIC

struct rlimit {
	rlim_t rlim_cur;
	rlim_t rlim_max;
};

struct robust_list {
  struct robust_list *next;
};

struct robust_list_head {
  struct robust_list list;
  long futex_offset;
  struct robust_list *list_op_pending;
};


// Per-process state
struct proc {
  int magic;
  struct spinlock lock;
  // p->lock must be held when using these:
  enum procstate state;        // Process state
  struct proc *parent;         // Parent process
  void *chan;                  // If non-zero, sleeping on chan
  // the signal that will be handled...
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;                     // Process ID
  int uid;                      
  int gid;

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  int64 filelimit;
  struct file **ofile;        // Open files
  int *exec_close;        // Open files
  struct dirent *cwd;          // Current directory
  char name[20];               // Process name (debugging)
  int tmask;                    // trace mask
  uint64 sleep_expire;			// wake up time for sleeping
  struct tms proc_tms;
  struct list dlist;
  struct list c_list;
  struct list sib_list;
  struct vma *vma;
  uint64 q;
  map_fix *mf;
  // signal
  ksigaction_t *sig_act;//action
  __sigset_t sig_set;// proc_mask //ignore
  __sigset_t sig_pending;// pending xin hao deng dai dui lie
  struct sig_frame *sig_frame;// xin hao ban de trapframe
  uint64 set_child_tid;
  uint64 clear_child_tid;
  struct robust_list_head *robust_list;
  int umask;
  int vswtch;
  int ivswtch;
  int epc_num;
  uint64 epc_nums[5];
};


static inline struct proc* sib_getproc(struct list* list){
  return dlist_entry(list,struct proc,sib_list);
}

#define NOFILEMAX(p) (p->filelimit<NOFILE?p->filelimit:NOFILE)

void            exit(int);
int             fork(void);
void            forkret(void);
pagetable_t     proc_pagetable(struct proc *p, struct proc *pp, int thread_create);
// void            proc_freepagetable(pagetable_t, uint64);
void            proc_freepagetable(struct proc *p);
int             kill(int pid,int sig);
int             tgkill(int pid,int tid,int sig);
struct proc*    myproc();
void            procinit(void);
void            scheduler(void) __attribute__((noreturn));
void            sched(void);
void            setproc(struct proc*);
void            sleep(void*, struct spinlock*);
void            userinit(void);
void            getcharinit(void);
int             wait(uint64);
int             wait4pid(int, uint64);
int             wakeup(void*);
void            yield(void);
void            procdump(void);
uint64          procnum(void);
struct proc*    getparent(struct proc* child);
void            allocparent(struct proc* parent,struct proc* child);
void            test_proc_init(int);
int             clone(uint64 flag, uint64 stack, uint64 ptid, uint64 tls, uint64 ctid);
void            proc_tick(void);
struct proc*    findproc(int pid);
int             do_futex(int* uaddr,int futex_op,int val,ktime_t *timeout,int *addr2,int val2,int val3);

#endif
