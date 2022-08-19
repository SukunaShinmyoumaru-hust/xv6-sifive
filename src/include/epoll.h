#ifndef __EPOLL_H
#define __EPOLL_H

#include "types.h"
#include "timer.h"
#include "spinlock.h"
#include "signal.h"
#include "file.h"
#include "utils/list.h"
#include "utils/waitqueue.h"

#define EPOLLIN 0x001
#define EPOLLPRI 0x002
#define EPOLLOUT 0x004
#define EPOLLRDNORM 0x040
#define EPOLLNVAL 0x020
#define EPOLLRDBAND 0x080
#define EPOLLWRNORM 0x100
#define EPOLLWRBAND 0x200
#define EPOLLMSG 0x400
#define EPOLLERR 0x008
#define EPOLLHUP 0x010
#define EPOLLRDHUP 0x2000
#define EPOLLEXCLUSIVE (1U<<28)
#define EPOLLWAKEUP (1U<<29)
#define EPOLLONESHOT (1U<<30)
#define EPOLLET (1U<<31)

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

typedef union epoll_data {
   void        *ptr;
   int          fd;
   uint32       u32;
   uint64       u64;
} epoll_data_t;

struct epoll_event {
   uint32     events;      /* Epoll events */
   epoll_data_t data;        /* User data variable */
};

struct epoll_fd{
   int fd;
   struct epoll_event events;
};

#define EPOLLSIZE 100
struct epoll{
	struct epoll_fd pool[EPOLLSIZE];
	
};

struct epoll* epollalloc();
int epolladd(struct epoll* epoll, int fd,struct epoll_event* events);
int epolldel(struct epoll* epoll, int fd);
void epollclose(struct epoll* epoll);
int epoll_pwait(struct epoll* epoll,uint64 events_addr,int maxevents,__sigset_t* sigmask, int timeout);
#endif
