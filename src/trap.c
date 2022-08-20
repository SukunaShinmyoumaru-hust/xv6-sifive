#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/vm.h"
#include "include/sbi.h"
#include "include/plic.h"
#include "include/trap.h"
#include "include/syscall.h"
#include "include/printf.h"
#include "include/console.h"
#include "include/timer.h"
#include "include/disk.h"

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
extern void kernelvec();

int devintr();
int handle_excp(uint64 scause);

// Interrupt flag: set 1 in the Xlen - 1 bit
#define INTERRUPT_FLAG    0x8000000000000000L

// Supervisor interrupt number
#define INTR_SOFTWARE    (0x1 | INTERRUPT_FLAG)
#define INTR_TIMER       (0x5 | INTERRUPT_FLAG)
#define INTR_EXTERNAL    (0x9 | INTERRUPT_FLAG)

// Supervisor exception number
#define EXCP_LOAD_ACCESS  0x5
#define EXCP_STORE_ACCESS 0x7
#define EXCP_ENV_CALL     0x8
#define EXCP_LOAD_PAGE    0xd // 13
#define EXCP_STORE_PAGE   0xf // 15

// void
// trapinit(void)
// {
//   initlock(&tickslock, "time");
//   #ifdef DEBUG
//   printf("trapinit\n");
//   #endif
// }
static void print_epc_info(){
  struct proc* p = myproc();
  for(int i = 0;i < 5;i++){
    printf("epc %d : %p\n",i,p->epc_nums[(p->epc_num - 1 + i) % 5]);
  }
}
// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
  w_sstatus(r_sstatus() | SSTATUS_SIE);
  // enable supervisor-mode timer interrupts.
  w_sie(r_sie() | SIE_SEIE | SIE_SSIE | SIE_STIE);
  set_next_timeout();
  __debug_info("trapinithart\n");
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;
  int ret = 0;
  // printf("[usertrap] into usertrap\n");

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");
  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  //printf("user trap scause:%p\n",r_scause());
  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  //printf("[usertrap] enter epc:%p\n",p->trapframe->epc);
  
  uint64 cause = r_scause();
  if(cause == EXCP_ENV_CALL){
    // system call
    if(p->killed == SIGTERM)
      exit(-1);
    // sepc points to the ecall 1instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;
    // an interrupt will change sstatus &c registers,
    // so don't enable until done with those registers.
    intr_on();
    syscall();
  } 
  else if((which_dev = devintr()) != 0){
    // ok
  }
  else if((ret = handle_excp(cause)) != -2)
  {
    // __debug_info("[usertrap] handle pagefault ret = %d\n", ret);
    if(ret == -1)
    {
      send_signal(SIGSEGV);
    }
  }
  else if(cause == 3){
    // printf("ebreak\n");
    // trapframedump(p->trapframe);
    p->trapframe->epc += 2;
  }
  else {
  	printf("\nusertrap(): unexpected scause %p pid=%d %s\n", r_scause(), p->pid, p->name);
        printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
        trapframedump(p->trapframe);
        p->killed = SIGTERM;
        print_epc_info();
  }

  if (p->killed) {
		if (SIGTERM == p->killed)
			exit(-1);
		// __debug_info("[usertrap] enter handler\n");
		sighandle();
  }


  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2){
    yield();
    p->ivswtch += 1;
  }
    
  //printf("[usertrap] end epc:%p\n",p->trapframe->epc);
  usertrapret();
}


void
usertrapret(){
  struct proc *p = myproc();
  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();
  // send syscalls, interrupts, and exceptions to trampoline.S
  w_stvec(TRAMPOLINE + (uservec - trampoline));

  // set up trapframe values that uservec will need when
  // the process next re-enters the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  p->epc_nums[p->epc_num] = p->trapframe->epc;
  p->epc_num = (p->epc_num + 1) % 5;
  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  // printf("[usertrapret]p->pagetable: %p\n", p->pagetable);
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.

  uint64 fn = TRAMPOLINE + (userret - trampoline);
  
  ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}

void 
kerneltrap() {
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  // printf("kerneltrap scause:%p\n",scause);
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    printf("\nscause %p\n", scause);
    printf("sepc=%p stval=%p hart=%d\n", r_sepc(), r_stval(), r_tp());
    struct proc *p = myproc();
    if (p != 0) {
      printf("pid: %d, name: %s\n", p->pid, p->name);
    }
    panic("kerneltrap");
  }
  // printf("which_dev: %d\n", which_dev);
  
  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING) {
    //printf("[kerneltrap] hart %d time interrupt\n",mycpu()-cpus);
    yield();
  }
  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

// Check if it's an external/software interrupt, 
// and handle it. 
// returns  2 if timer interrupt, 
//          1 if other device, 
//          0 if not recognized. 
int devintr(void) {
	uint64 scause = r_scause();
	//printf("devintr scause:%p\n",scause);

	// handle external interrupt 
	if ((0x8000000000000000L & scause) && 9 == (scause & 0xff)) 
	{
		int irq =0;
		// plic_claim();
		printf("irq:%d\n",irq);
		if (UART0_IRQ == irq) {
			//printf("cao\n");
			// keyboard input 
			int c = sbi_console_getchar();
			if (-1 != c) {
				//consoleintr(c);
			}
		}
		/*
		else if (DISK_IRQ == irq) {
			disk_intr();
		}
		*/
		else if (irq) {
			printf("unexpected interrupt irq = %d\n", irq);
		}

		if (irq) { 
		  //plic_complete(irq);
		}

		#ifndef QEMU 
		w_sip(r_sip() & ~2);    // clear pending bit
		sbi_set_mie();
		#endif 

		return 1;
	}
	else if (0x8000000000000005L == scause) {
		timer_tick();
                proc_tick();
		return 2;
	}
	else { return 0;}
}

// be noticed that syscall is not handled here 
int handle_excp(uint64 scause) {
	// later implementation may handle more cases, such as lazy allocation, mmap, etc.
	switch (scause) {
	case EXCP_STORE_PAGE: 
	#ifndef QEMU 
	case EXCP_STORE_ACCESS: 
	#endif 
		return handle_page_fault(1, r_stval());
	case EXCP_LOAD_PAGE: 
	#ifndef QEMU 
	case EXCP_LOAD_ACCESS: 
	#endif 
		return handle_page_fault(0, r_stval());
  default: 
    return -2;
	}
}


void trapframedump(struct trapframe *tf)
{
  printf("a0: %p\t", tf->a0);
  printf("a1: %p\t", tf->a1);
  printf("a2: %p\t", tf->a2);
  printf("a3: %p\n", tf->a3);
  printf("a4: %p\t", tf->a4);
  printf("a5: %p\t", tf->a5);
  printf("a6: %p\t", tf->a6);
  printf("a7: %p\n", tf->a7);
  printf("t0: %p\t", tf->t0);
  printf("t1: %p\t", tf->t1);
  printf("t2: %p\t", tf->t2);
  printf("t3: %p\n", tf->t3);
  printf("t4: %p\t", tf->t4);
  printf("t5: %p\t", tf->t5);
  printf("t6: %p\t", tf->t6);
  printf("s0: %p\n", tf->s0);
  printf("s1: %p\t", tf->s1);
  printf("s2: %p\t", tf->s2);
  printf("s3: %p\t", tf->s3);
  printf("s4: %p\n", tf->s4);
  printf("s5: %p\t", tf->s5);
  printf("s6: %p\t", tf->s6);
  printf("s7: %p\t", tf->s7);
  printf("s8: %p\n", tf->s8);
  printf("s9: %p\t", tf->s9);
  printf("s10: %p\t", tf->s10);
  printf("s11: %p\t", tf->s11);
  printf("ra: %p\n", tf->ra);
  printf("sp: %p\t", tf->sp);
  printf("gp: %p\t", tf->gp);
  printf("tp: %p\t", tf->tp);
  printf("epc: %p\n", tf->epc);
}
