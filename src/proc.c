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
#include "include/copy.h"
#include "include/file.h"
#include "include/trap.h"
#include "include/vm.h"
#include "include/vma.h"
#include "include/mmap.h"
#include "include/pm.h"
#include "include/errno.h"

#define WAITQ_NUM 100

extern pagetable_t kernel_pagetable;
extern void swtch(struct context*, struct context*);
struct proc proc[NPROC];

struct proc *initproc;
struct proc *runproc;
queue readyq;
struct spinlock waitq_pool_lk;
queue waitq_pool[WAITQ_NUM];
int waitq_valid[WAITQ_NUM];
int firstuserinit;

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
  firstuserinit = 1;
  for(struct proc* p = proc;p<proc+NPROC;p++){
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->parent = NULL;
      p->killed = 0;
      p->filelimit = NOFILE;
      p->umask = 0;
  }
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



struct proc*
findproc(int pid)
{
  for(struct proc* p = proc;p!=proc+NPROC;p++){
    if(p->pid == pid&& p->state !=UNUSED){
      return p;
    }
  }
  return NULL;
}


// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
/*
  if(p->trapframe)
    freepage((void*)p->trapframe);
    */
  //__debug_warn("[freeproc]free pid %d\n",p->pid);
  p->trapframe = 0;
  if(p->mf)
    free_map_fix(p);
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
  list_del(&p->sib_list);
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
allocproc(struct proc *pp, int thread_create)
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
  p->umask = 0;
  p->pid = allocpid();
  p->killed = 0;
  p->mf = NULL;
  p->filelimit = NOFILE;
  p->robust_list = NULL;
  p->clear_child_tid = NULL;
  p->set_child_tid = NULL;
  p->vma = NULL;
  p->uid = 0;
  p->gid = 0;
  p->vswtch = 0;
  p->ivswtch = 0;
  p->sleep_expire = 0;
  p->q = NULL;
  // times for process performance 
  p->proc_tms.utime = 0;
  p->proc_tms.stime = 0;
  p->proc_tms.cutime = 0;
  p->proc_tms.cstime = 0;
  list_init(&p->c_list);
  list_init(&p->sib_list);
  // Allocate a trapframe page.
  if((p->trapframe = allocpage()) == NULL){
    release(&p->lock);
    return NULL;
  }

  p->kstack = (uint64)allocpage();
  
  
  // An empty user page table.
  // And an identical kernel page table for this proc.
  if ((proc_pagetable(p, pp, thread_create)) == NULL) {
    freeproc(p);
    release(&p->lock);
    return NULL;
  }
  p->ofile = kmalloc(NOFILE*sizeof(struct file*));
  p->exec_close = kmalloc(NOFILE*sizeof(int));
  
  if(!p->ofile){
    panic("proc ofile init\n");
  }
  
  for(int fd = 0; fd < NOFILE; fd++){
    p->ofile[fd] = NULL;
    p->exec_close[fd] = 0;
  }
  p->epc_num = 0;
  memset(p->epc_nums,0,sizeof(uint64)*5);
  memset(p->ofile, 0, NOFILE*sizeof(struct file*));
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
proc_pagetable(struct proc *p, struct proc *pp, int thread_create)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = kvmcreate();
  if(pagetable == 0)
    return NULL;

  p->pagetable = pagetable;
  //printf("[proc pagetable]alloc pagetable:%p\n",p->pagetable);
  if(pp == NULL)
  {
    
    if(vma_list_init(p) == NULL)
    {
      //__debug_warn("[proc_pagetable] vma list init failed\n");
      freewalk(pagetable);
      p->pagetable = NULL;
      return NULL;
    }
  }
  else
  {
    struct vma *nvma = NULL;
    if((nvma = vma_copy(p, pp->vma)) == NULL)
    {
      //__debug_warn("[proc_pagetable] vma copy fail\n");
      freepage(pagetable);
      p->pagetable = NULL;
      return NULL;
    }
    nvma = nvma->next;
    if(thread_create)
    {
      while(nvma != p->vma)
      {
        if(nvma->type != TRAP && vma_shallow_mapping(pp->pagetable, p->pagetable, nvma) < 0)
        {
          __debug_warn("[proc_pagetable] vma shallow mapping fail\n");
          free_vma_list(p);
          freepage(pagetable);
          p->pagetable = NULL;
          return NULL;
        }
        nvma = nvma->next;
      }
    }
    else
    {
      while(nvma != p->vma)
      {
        if(nvma->type != TRAP && vma_deep_mapping(pp->pagetable, p->pagetable, nvma) < 0)
        {
          __debug_warn("[proc_pagetable] vma deep mapping fail\n");
          free_vma_list(p);
          freepage(pagetable);
          p->pagetable = NULL;
          return NULL;
        }
        nvma = nvma->next;
      }
    }
  }
  //print_vma_info(p);
  //__debug_warn("[proc_pagetable] successfully return\n");
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
  p = allocproc(0, 0);
  if(firstuserinit){
    initproc = p;
    firstuserinit = 0;
  }
  else{
    allocparent(p,initproc);
    initproc = p;
  }
  alloc_load_vma(p, (uint64) 0, initcodesize, PTE_R|PTE_W|PTE_X|PTE_U);
  //print_vma_info(p);
  copyout(p->pagetable,0,initcode,initcodesize);
  
  p->trapframe->epc = 0x0;      // user program counter
  p->trapframe->sp = type_locate_vma(p->vma,STACK)->end;  // user stack pointer
  
  safestrcpy(p->name, "initcode", sizeof(p->name));
  
  p->state = RUNNABLE;
  readyq_push(p);//insert to ready queue
  p->tmask = 0;
  p->cwd = ename(NULL,"/",0);
  release(&p->lock);
  __debug_info("userinit\n");
}

int clone(uint64 flag, uint64 stack, uint64 ptid, uint64 tls, uint64 ctid) {
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();


  if((flag & CLONE_THREAD) && (flag & CLONE_VM))
  {
    // Allocate process.
    if((np = allocproc(p, 1)) == NULL){
      __debug_warn("alloc process bad\n");
      return -1;
    }
    
    // copy saved user registers.
    *(np->trapframe) = *(p->trapframe);
    np->trapframe->tp = tls;
    np->trapframe->sp = stack;
    if(ptid != 0)
    {
      copyout(p->pagetable, ptid, (char *)&np->pid, sizeof(int));
    } 
  }
  else
  {
    //__debug_info("[clone]enter\n");
    // Allocate process.
    if((np = allocproc(p, 0)) == NULL){
      return -1;
    }
    // copy saved user registers.
    *(np->trapframe) = *(p->trapframe);
    if(stack != 0)
    {
      p->trapframe->sp = stack;
    }
  }
  
  // signal copy	
  sigaction_copy(&np->sig_act, p->sig_act);
  np->sig_frame = p->sig_frame;
  for (int i = 0; i < SIGSET_LEN; i++) {
    np->sig_pending.__val[i] = p->sig_pending.__val[i];
  }
  
  np->sz = p->sz;

  // copy tracing mask from parent.
  np->tmask = p->tmask;

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;
  

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = edup(p->cwd);
  
  // np->parent = p;
  allocparent(p, np);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;
  np->state = RUNNABLE;
  readyq_push(np);

  if(flag & CLONE_CHILD_SETTID){
    np->set_child_tid = ctid;

  }
  if(flag & CLONE_CHILD_CLEARTID){
    np->clear_child_tid = ctid;
  }
  
  p->killed = np->killed;
  
  release(&np->lock);

  // __debug_info("[clone] pid %d ", pid);
  // print_free_page_n();
  //__debug_info("epc:%p\n",p->trapframe->epc);
  //__debug_info("[clone] pid %d clone pid %d\n",pid,p->pid);
  return pid;
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
  p->vswtch += 1;
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
int
wakeup(void *chan)
{
   queue* q = findwaitq(chan);
   if(q){
     struct proc* p;
     while((p = waitq_pop(q))!=NULL){
       p->state = RUNNABLE;
       readyq_push(p);
     }
     delwaitq(q);
     return 1;
   }else{
     return 0;
   }
}

void
proc_tick()
{
  uint64 now = r_time();
  queue tmp;
  queue_init(&tmp, NULL);
  acquire(&waitq_pool_lk);
  for(int i=0;i<WAITQ_NUM ;i++){
    if(!waitq_valid[i])continue;
    queue* q = waitq_pool+i;
    qlock(q);
    struct list* head = &q->head;
    struct list* it = list_next(head);
    while(it!=head){
      struct proc* p = dlist_entry(it, struct proc, dlist);
      struct list* nextit = list_next(it);
      if (p->sleep_expire && now >= p->sleep_expire) {
        p->sleep_expire = 0;
        list_del(it);
        list_add_before(&tmp.head,it);
      }
      it = nextit;
    }
    if(list_empty(&q->head)){
      waitq_valid[i]=0;
    }
    qunlock(q);
  }
  release(&waitq_pool_lk);
  struct list* head = &tmp.head;
  struct list* it = list_next(head);
  while(it!=head){
      struct proc* p = dlist_entry(it, struct proc, dlist);
      struct list* nextit = list_next(it);
      list_del(it);
      p->state = RUNNABLE;
      readyq_push(p);
      it = nextit;
  }
}

void
allocparent(struct proc* parent,struct proc* child){
  child->parent = parent;
  list_add_after(&parent->c_list,&child->sib_list);
}


// get p's parent
// Caller must hold child->lock.
struct proc*
getparent(struct proc* child){
  return child->parent;
}

//need release return proc lock
struct proc*
findchild(struct proc* p,int (*cond)(struct proc*,int),int pid,struct proc** child){
   struct list* c_head = &p->c_list;
   struct list* c_it = list_next(c_head);
   *child = NULL;
   struct proc* np = NULL;
   while(c_it!=c_head){
      np = sib_getproc(c_it);
      //printf("[findchild]pid %d find child pid %d\n",p->pid,np->pid);
      // this code uses np->parent without holding np->lock.
      // acquiring the lock first would cause a deadlock,
      // since np might be an ancestor, and we already hold p->lock.
      if(cond(np,pid)){
        // np->parent can't change between the check and the acquire()
        // because only the parent changes it, and we're the parent.
        acquire(&np->lock);
        *child = np; 
        if(np->state == ZOMBIE){
          return np;
        }
        release(&np->lock);
      }
      c_it = list_next(c_it);
   }
   return NULL;
}

// Pass p's abandoned children to init.
// Caller must hold p->lock.
void
reparent(struct proc *p)
{
  //__debug_warn("pid %d reparent\n",p->pid);
  struct list *c_head = &p->c_list;
  if(list_empty(c_head))return;
  struct list *c_next = list_next(c_head);
  struct list *c_prev = list_prev(c_head);
  struct list *c_it = c_next;
  struct proc *pp = NULL;
  while(c_it!=c_head){
    pp = sib_getproc(c_it);
    // this code uses pp->parent without holding pp->lock.
    // acquiring the lock first could cause a deadlock
    // if pp or a child of pp were also in exit()
    // and about to try to lock p.
      // pp->parent can't change between the check and the acquire()
      // because only the parent changes it, and we're the parent.
      acquire(&pp->lock);
      pp->parent = initproc;
      // we should wake up init here, but that would require
      // initproc->lock, which would be a deadlock, since we hold
      // the lock on one of init's children (pp). this is why
      // exit() always wakes init (before acquiring any locks).
      release(&pp->lock);
      c_it = list_next(c_it);
  };
  struct list* init_head = &initproc->c_list;
  struct list* init_next = list_next(init_head);
  __list_link(init_head,c_next);
  __list_link(c_prev,init_next);
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


int zombiecond(struct proc* p,int pid){
  return (pid==-1||p->pid == pid);
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait4pid(int pid,uint64 addr)
{
  int kidpid;
  struct proc *p = myproc();
  struct proc* child;
  struct proc* chan = NULL;
  // hold p->lock for the whole time to avoid lost
  // wakeups from a child's exit().
  acquire(&p->lock);
  //__debug_warn("[wait4pid]pid%d:%s enter\n",p->pid,p->name);
  while(1){
    kidpid = pid;
    child = findchild(p,zombiecond,pid,&chan);
    //__debug_warn("[wait4pid]pid%d:%s chan:%p\n",p->pid,p->name,chan);
    if(child != NULL){
      kidpid = child->pid;
      p->proc_tms.cstime += child->proc_tms.stime + child->proc_tms.cstime;
      p->proc_tms.cutime += child->proc_tms.utime + child->proc_tms.cutime;
      child->xstate <<= 8;
      if(addr != 0 && copyout(p->pagetable, addr, (char *)&child->xstate, sizeof(child->xstate)) < 0) {
        release(&child->lock);
        release(&p->lock);
        __debug_warn("[wait4pid]pid%d:%s copyout bad\n",p->pid,p->name);
        return -1;
      }
      freeproc(child);
      release(&child->lock);
      release(&p->lock);
      // __debug_info("[wait4pid] pid %d ", pid);
      // print_free_page_n();
      //__debug_info("[wait4pid]pid%d:%s kidpid:%p\n",p->pid,p->name,kidpid);
      return kidpid;
    }
    if(!chan){
      //__debug_warn("[wait4pid]pid%d:%s no kid to wait\n",p->pid,p->name);
      release(&p->lock);
      return -1;
    }
    if(pid == -1)sleep(p, &p->lock);  //DOC: wait-sleep
    else sleep(chan,&p->lock);
  }
  release(&p->lock);
  return 0;
}

void
exit(int n)
{
  struct proc *p = myproc();
  //if(p == initproc)
    //panic("init exiting");
  //__debug_warn("[exit]pid %d:%s exit %d\n",p->pid,p->name,n);
  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }
  if(p->ofile)
    kfree((void*)p->ofile);
  if(p->exec_close)
    kfree((void*)p->exec_close);
  p->ofile = 0;

  eput(p->cwd);
  p->cwd = 0;
  wakeup(p);
  acquire(&p->lock);
  wakeup(getparent(p));
  reparent(p);
  
  p->xstate = n;
  p->state = ZOMBIE;
  
  // p->killed = SIGTERM;
  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
  while(1){
  
  }
}

int kill(int pid,int sig){
	struct proc* p;
	for(p = proc; p < &proc[NPROC]; p++){
		if(p->pid == pid){
			acquire(&p->lock);
			if(p->state == SLEEPING){
				// need to modify...
				queue_del(p);
				readyq_push(p);
				p->state = RUNNABLE;
			}
			p->sig_pending.__val[0] |= 1ul << sig;
			if (0 == p->killed || sig < p->killed) {
				p->killed = sig;
			}
			release(&p->lock);
			return 0;
		}
	}
  //return -ESRCH;
  return 0;
}

static int cmp_parent(int pid,int sid){
  struct proc* p;
  for(p = proc;p < &proc[NPROC];p++){
    if(p->pid == sid) break;
  }
  while(p){
    p = getparent(p);
    if(!p)break;
    if(p->pid == pid) return 1;
  }
  return 0;
}

int tgkill(int pid,int tid,int sig){
  if(!cmp_parent(pid,tid)) return -1;
  else return kill(tid,sig);
}
