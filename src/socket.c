#include "include/types.h"
#include "include/riscv.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/timer.h"
#include "include/kalloc.h"
#include "include/string.h"
#include "include/printf.h"
#include "include/kalloc.h"
#include "include/console.h"
#include "include/string.h"
#include "include/stat.h"
#include "include/vm.h"
#include "include/uname.h"
#include "include/socket.h"



struct socket*
socketalloc()
{
  struct socket* sk = kmalloc(sizeof(struct socket));
  memset(sk,0,sizeof(struct socket));
  initlock(&sk->lk,"socket");
  return sk;
}

void
socketclose(struct socket* sk)
{
  acquire(&sk->lk);
  release(&sk->lk);
  kfree(sk);
}

void
socketkstat(struct socket* sk, struct kstat* kst)
{
  
}

int
socketread(struct socket* sk, int user, uint64 addr, int n)
{
  __debug_warn("socket read addr:%p n:%p\n", addr, n);
  return 0;
}

int
socketwrite(struct socket* sk, int user, uint64 addr, int n)
{
  __debug_warn("socket write addr:%p n:%p\n", addr, n);
  return 0;
}

void
socketbind(struct socket* sk,struct sockaddr* addr)
{
/*
  acquire(&sk->lk);
  addr->next = sk->addr;
  sk->addr = addr;
  release(&sk->lk);
  */
}

void
print_sockaddr(struct sockaddr* addr){
  printf("sockaddr family:%p\tport:%p\taddr:%p\n",addr->sin_family,addr->sin_port,addr->sin_addr);
}
