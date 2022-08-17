#include "include/types.h"
#include "include/errno.h"
#include "include/riscv.h"
#include "include/epoll.h"
#include "include/proc.h"
#include "include/vm.h"
#include "include/timer.h"
#include "include/string.h"
#include "include/printf.h"
#include "include/kalloc.h"
#include "include/utils/waitqueue.h"

struct epoll*
epollalloc()
{
  struct epoll* epoll = kmalloc(sizeof(struct epoll));
  for(int i = 0;i<EPOLLSIZE;i++){
    epoll->pool[i].fd = -1;
  }
  return epoll;
}

void
epolladd(struct epoll* epoll, int fd,struct epoll_event* events)
{
  for(int i=0;i<EPOLLSIZE;i++){
    if(epoll->pool[i].fd == -1){
      epoll->pool[i].fd = fd;
      epoll->pool[i].events = *events;
    }
  }
}

void
epolldel(struct epoll* epoll, int fd)
{
  for(int i=0;i<EPOLLSIZE;i++){
    if(epoll->pool[i].fd == fd){
      epoll->pool[i].fd = -1;
    }
  }
}

void
epollclose(struct epoll* epoll)
{
  kfree(epoll);
}

