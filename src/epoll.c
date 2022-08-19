#include "include/types.h"
#include "include/errno.h"
#include "include/riscv.h"
#include "include/poll.h"
#include "include/epoll.h"
#include "include/proc.h"
#include "include/vm.h"
#include "include/timer.h"
#include "include/string.h"
#include "include/copy.h"
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

int
epolladd(struct epoll* epoll, int fd,struct epoll_event* events)
{
  for(int i=0;i<EPOLLSIZE;i++){
    if(epoll->pool[i].fd == -1){
      epoll->pool[i].fd = fd;
      epoll->pool[i].events = *events;
      return 0;
    }
  }
  return -1;
}

int
epolldel(struct epoll* epoll, int fd)
{
  for(int i=0;i<EPOLLSIZE;i++){
    if(epoll->pool[i].fd == fd){
      epoll->pool[i].fd = -1;
      return 0;
    }
  }
  return -1;
}

void
epollprint(struct epoll* epoll)
{
  for(int i=0;i<EPOLLSIZE;i++){
    struct epoll_fd* fd = epoll->pool+i;
    if(fd->fd == -1)continue;
    printf("[EPOLL]i:%d fd:%d events:%p",i,fd->fd,fd->events.events);
    print_f_info(myproc()->ofile[fd->fd]);
  }
}

void
epollclose(struct epoll* epoll)
{
  kfree(epoll);
}


static uint32 file_epoll(struct file *fp, struct poll_table *pt)
{
	if (!fp->epollv)
		return EPOLLIN|EPOLLOUT;
	return fp->epollv(fp,pt);
}

int
epoll_pwait(struct epoll* epoll,uint64 events_addr,int maxevents,__sigset_t* sigmask, int timeout)
{
  epollprint(epoll);
  struct poll_wait_queue wait;
  struct proc* p = myproc();
  // struct poll_wait_queue *pwait;
  int immediate = 0;
  if (timeout == 0)
    immediate = 1;
  else 
  //expire += readtime();	// overflow?
    timeout += r_time();	// overflow?
  
  poll_init(&wait);
  if (immediate)
    wait.pt.func = NULL;		// we won't be inserted into any queue in later poll()s

  int ret = 0;
  struct epoll_event epoll_event;
  while(1){
    printf("wait loop enter\n");
    for(int i=0;i<EPOLLSIZE;i++){
      struct epoll_fd* fd = epoll->pool+i;
      if(fd->fd<0||fd->fd>=NOFILEMAX(p)||!p->ofile[fd->fd])continue;
      uint32 events = file_epoll(p->ofile[fd->fd], &wait.pt);
      events&=fd->events.events;
      if(events){
        wait.pt.func = NULL;
        epoll_event = (struct epoll_event){
          .events = events,
          .data = fd->events.data
        };
        printf("fd %d ok with events:%p\n", fd->fd, events);
        if(either_copyout(1
        		, events_addr+ret*sizeof(struct epoll_event)
        		, &epoll_event
        		, sizeof(struct epoll_event))<0){
           return -1;		
        }
        ret++;
      }
    }
    wait.pt.func = NULL;		// only need to be called once for each file
    if (ret > 0 || immediate)	// got results or don't sleep-wait
      break;
    if (wait.error) {
      ret = wait.error;
      break;
    }
    // at this point, maybe we are already waken up by some
    if (poll_sched_timeout(&wait, timeout))
      immediate = 1;
    if (myproc()->killed) {
      immediate = 1;
      wait.error = -EINTR;
    }
  }
  poll_end(&wait);
  printf("epoll pwait leave:%d\n",ret);
  /*
  while(1){
  
  }
  */
  return ret;
}
