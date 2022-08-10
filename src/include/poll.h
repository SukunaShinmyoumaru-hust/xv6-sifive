#ifndef __POLL_H
#define __POLL_H

#include "types.h"
#include "timer.h"
#include "spinlock.h"
#include "signal.h"
#include "file.h"
#include "utils/list.h"
#include "queue.h"


#define POLLIN      0x0001
#define POLLPRI     0x0002
#define POLLOUT     0x0004
#define POLLERR     0x0008
#define POLLHUP     0x0010
#define POLLNVAL    0x0020

#define POLLIN_SET	(POLLIN | POLLHUP | POLLERR)
#define POLLOUT_SET	(POLLOUT | POLLERR)
#define POLLEX_SET	POLLPRI


// Argument for poll().
struct pollfd {
	int32 fd;         /* file descriptor */
	int16 events;     /* requested events */
	int16 revents;    /* returned events */
};


#endif
