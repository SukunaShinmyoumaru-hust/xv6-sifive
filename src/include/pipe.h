#ifndef __PIPE_H
#define __PIPE_H

#include "types.h"
#include "spinlock.h"
#include "utils/waitqueue.h"
#include "file.h"

#define PIPE_SIZE 512


struct pipe {
	struct spinlock		lock;
	struct wait_queue	wqueue;
	struct wait_queue	rqueue;
	uint	nread;			// number of bytes read
	uint	nwrite;			// number of bytes written
	uint8	readopen;		// read fd is still open
	uint8	writeopen;		// write fd is still open
	uint8	writing;
	uint8 	size_shift;		// shift bit of pipe size
	char	*pdata;			// used for extended pipe 
	char	data[PIPE_SIZE];
	// char	data[];
};

#define PIPESIZE(pi)	(PIPE_SIZE << (pi->size_shift))
int pipealloc(struct file **f0, struct file **f1);
void pipeclose(struct pipe *pi, int writable);
int pipewrite(struct pipe *pi, int user, uint64 addr, int n);
int piperead(struct pipe *pi,int user, uint64 addr, int n);
int pipewritev(struct pipe *pi, struct iovec ioarr[], int count);
int pipereadv(struct pipe *pi, struct iovec ioarr[], int count);
#endif
