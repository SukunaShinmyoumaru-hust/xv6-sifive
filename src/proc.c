#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/queue.h"
#include "include/intr.h"
#include "include/kalloc.h"
#include "include/printf.h"
#include "include/string.h"
#include "include/file.h"
#include "include/trap.h"
#include "include/vm.h"
#include "include/vma.h"
#include "include/pm.h"

#define WAITQ_NUM 10

extern pagetable_t kernel_pagetable;
extern void swtch(struct context*, struct context*);
struct proc proc[NPROC];

struct proc *initproc;
struct proc *runproc;
queue readyq;
struct spinlock waitq_pool_lk;
queue waitq_pool[10];
int waitq_valid[10];

int nextpid = 1;
int procfirst = 1;
struct spinlock pid_lock;

extern char trampoline[]; // trampoline.S
extern char userret[];
extern char sig_trampoline[]; // trampoline.S
extern char initcode[]; // trampoline.S
extern int initcodesize;

void
waitq_pool_init(){
  for(int i = 0;i<WAITQ_NUM;i++){
    waitq_valid[i] = 0;
  }
  initlock(&waitq_pool_lk,"waitq pool");
}

void
procinit(){
  initlock(&pid_lock,"pid lock");
  initproc = NULL;
  queue_init(&readyq,NULL);
  waitq_pool_init();
  __debug_info("procinit\n");
}

queue*
findwaitq(void* chan){
  acquire(&waitq_pool_lk);
  for(int i=0;i<WAITQ_NUM ;i++){
    if(waitq_valid[i]&&waitq_pool[i].chan == chan){
      release(&waitq_pool_lk);
      return waitq_pool+i;
    }
  }
  release(&waitq_pool_lk);
  return NULL;
}

queue*
allocwaitq(void* chan){
  acquire(&waitq_pool_lk);
  for(int i=0;i<WAITQ_NUM ;i++){
    if(!waitq_valid[i]){
      waitq_valid[i] = 1;
      queue_init(waitq_pool+i,chan);
      release(&waitq_pool_lk);
      return waitq_pool+i;
    }
  }
  release(&waitq_pool_lk);
  return NULL;
}

void
delwaitq(queue* q){
  acquire(&waitq_pool_lk);
  int i = q - waitq_pool;
  waitq_valid[i] = 0;
  release(&waitq_pool_lk);
}

void
readyq_push(struct proc* p){
  queue_push(&readyq,p);
}

struct proc*
readyq_pop(){
  return queue_pop(&readyq);
}

void
waitq_push(queue *q,struct proc* p){
  queue_push(q,p);
}

struct proc*
waitq_pop(queue *q){
  return queue_pop(q);
}

void scheduler(){
  
  struct cpu *c = mycpu();
  c->proc = 0;
  while(1){
    struct proc* p = readyq_pop();  //...
    // printf("[scheduler]hart %d enter:%p\n",c-cpus,p);
    if(p){
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        // printf("[scheduler]found runnable proc with pid: %d\n", p->pid);
        p->state = RUNNING;
        c->proc = p;
        w_satp(MAKE_SATP(p->pagetable));
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
    }else{
      intr_on();
      asm volatile("wfi");
    }
  }
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
  if(p->kstack)
    freepage((void *)p->kstack);
  if(p->pagetable)
    proc_freepagetable(p);
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
  p->robust_list = NULL;
  p->clear_child_tid = NULL;
  p->set_child_tid = NULL;
  p->vma = NULL;
  // Allocate a trapframe page.
  if((p->trapframe = allocpage()) == NULL){
    release(&p->lock);
    return NULL;
  }

  p->kstack = (uint64)allocpage();
  
  
  // An empty user page table.
  // And an identical kernel page table for this proc.
  if ((proc_pagetable(p)) == NULL) {
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
  pagetable = kvmcreate();
  if(pagetable == 0)
    return NULL;

  p->pagetable = pagetable;

  if(vma_list_init(p) == NULL)
  {
    __debug_warn("[proc_pagetable] vma list init failed\n");
    p->pagetable = NULL;
    freewalk(pagetable);
    return NULL;
  }
  
  return pagetable;
}


// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(struct proc *p)
{
  uvmfree(p);
}

void
userinit()
{
  struct proc *p;

  p = allocproc();
  initproc = p;
  alloc_load_vma(p, (uint64) 0, initcodesize, PTE_R|PTE_W|PTE_X|PTE_U);
  print_vma_info(p);
  copyout(p->pagetable,0,initcode,initcodesize);
  

  p->trapframe->epc = 0x0;      // user program counter
  p->trapframe->sp = type_locate_vma(p->vma,STACK)->end;  // user stack pointer
  
  safestrcpy(p->name, "initcode", sizeof(p->name));
  
  p->state = RUNNABLE;
  readyq_push(p);//insert to ready queue
  p->tmask = 0;

  release(&p->lock);
  __debug_info("userinit\n");
}


// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  //printf("run in forkret\n");

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

void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.
  if(lk != &p->lock){  //DOC: sleeplock0
    acquire(&p->lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  queue* q = findwaitq(chan);
  if(!q)q = allocwaitq(chan);
  if(!q){
    __debug_error("waitq pool is full\n");
  }
  waitq_push(q,p);
  p->state = SLEEPING;
  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &p->lock){
    release(&p->lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
   queue* q = findwaitq(chan);
   if(q){
     struct proc* p;
     while((p = waitq_pop(q))!=NULL){
       readyq_push(p);
     }
     delwaitq(q);
   }
}

void
yield()
{
  struct proc *p = myproc();
  acquire(&p->lock);
  readyq_push(p);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

void
exit(int n)
{
   printf("[exit]exit %d\n",n);
   while(1){
   
   }
}


