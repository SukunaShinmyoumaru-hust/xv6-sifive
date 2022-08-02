#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/intr.h"
#include "include/kalloc.h"
#include "include/printf.h"
#include "include/string.h"
#include "include/file.h"
#include "include/trap.h"
#include "include/vm.h"


extern pagetable_t kernel_pagetable;
extern void swtch(struct context*, struct context*);
struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
int procfirst = 1;
struct spinlock pid_lock;

extern char trampoline[]; // trampoline.S
extern char sig_trampoline[]; // trampoline.S

void
procinit(){
  initlock(&pid_lock,"pid lock");
  initproc = NULL;
  __debug_info("procinit\n");
}

void scheduler(){
  
  struct cpu *c = mycpu();
  c->proc = 0;
  while(1){
    struct proc* p = initproc;
    if(p){
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        // printf("[scheduler]found runnable proc with pid: %d\n", p->pid);
        p->state = RUNNING;
        c->proc = p;
        w_satp(MAKE_SATP(kernel_pagetable));
        sfence_vma();
        swtch(&c->context, &p->context);
        w_satp(MAKE_SATP(kernel_pagetable));
        sfence_vma();
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;

        // found = 1;
      }
      release(&p->lock);
    }
  }
}

void
userinit()
{
  __debug_info("userinit\n");
}

int
allocpid() {
  int pid;
  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);
  return pid;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->ofile)
    kfree((void*)p->ofile);
  p->ofile = 0;

  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  //delvmas(p->vma);
  p->vma = NULL;
  // how to handle robust_list?
  p->robust_list = NULL;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;

  // free signal 
  sigaction_free(p->sig_act);

  // free the list of sig_frame 
  sigframefree(p->sig_frame);
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return NULL;

found:
  p->pid = allocpid();
  p->killed = 0;
  p->filelimit = NOFILE;
  p->vma = NULL;
  p->robust_list = NULL;
  p->clear_child_tid = NULL;
  p->set_child_tid = NULL;
  // Allocate a trapframe page.
  if((p->trapframe = kmalloc(sizeof(struct trapframe))) == NULL){
    release(&p->lock);
    return NULL;
  }

  // An empty user page table.
  // And an identical kernel page table for this proc.
  if ((p->pagetable = proc_pagetable(p)) == NULL) {
    freeproc(p);
    release(&p->lock);
    return NULL;
  }

  p->ofile = kmalloc(NOFILE*sizeof(struct file*));
  if(!p->ofile){
    panic("proc ofile init\n");
  }

  for(int fd = 0; fd < NOFILE; fd++){
    p->ofile[fd] = NULL;
  }
  memset(p->ofile, 0, PGSIZE);
/*
  for(int i = 0; i < MMAPNUM; ++i){
    p->mmap_pool[i].used = 0;
  }
  */

  p->kstack = VKSTACK;

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;
  p->proc_tms.utime = 0;
  p->proc_tms.stime = 0;
  p->proc_tms.cutime = 1;
  p->proc_tms.cstime = 1;

  p->sig_act = NULL;
  p->sig_frame = NULL;
  for (int i = 0; i < SIGSET_LEN; i ++) {
	p->sig_pending.__val[i] = 0;
  }
	p->killed = 0;
  return p;
}

// Create a user page table for a given process,
// with no user memory, but with trampoline pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return NULL;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X) < 0){
    uvmfree(pagetable, 0);
    return NULL;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, SIG_TRAMPOLINE, PGSIZE, (uint64)sig_trampoline, PTE_R | PTE_X | PTE_U) < 0){
    uvmfree(pagetable, 0);
    vmunmap(pagetable, TRAMPOLINE, 1, 0);
    return NULL;
  }

  // map the trapframe just below TRAMPOLINE, for trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W) < 0){
    vmunmap(pagetable, SIG_TRAMPOLINE, 1, 0);
    vmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return NULL;
  }


  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable,uint64 sz)
{
  vmunmap(pagetable, SIG_TRAMPOLINE, 1, 0);
  vmunmap(pagetable, TRAMPOLINE, 1, 0);
  vmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  // printf("run in forkret\n");

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);
  usertrapret();
}


uint64
procnum(void)
{
  int num = 0;
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->state != UNUSED) {
      num++;
    }
  }

  return num;
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{


}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{


}

void
exit(int n)
{

}


