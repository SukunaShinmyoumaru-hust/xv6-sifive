#ifndef __COPY_H
#define __COPY_H

#include "types.h"
#include "riscv.h"
#include "proc.h"
struct proc;

typedef struct{
  void* chan;
  struct spinlock lk;
  struct list head;
}queue;

static inline void queue_init(queue *q,void* chan) {
	initlock(&q->lk,"queue");
	list_init(&q->head);
	q->chan = chan;
}

static inline void qlock(queue *q){
	acquire(&q->lk);
}

static inline void qunlock(queue *q){
	release(&q->lk);
}

static inline int queue_empty(queue *q) {
	qlock(q);
	int ret = list_empty(&q->head);
	qunlock(q);
	return ret;
}

static inline void queue_push(queue* q,struct proc* p){
	qlock(q);
	list_add_before(&q->head,&p->dlist);
	p->q = (uint64)q;
	qunlock(q);
}

static inline struct proc* queue_pop(queue* q){
	struct proc* p = NULL;
	if(!queue_empty(q)){
		qlock(q);
		struct list* l = list_next(&q->head);
		list_del(l);
		p = dlist_entry(l, struct proc, dlist);
		p->q = 0;
		qunlock(q);
	}
	return p;
}

static inline int queue_del(struct proc* p){
	queue* q = (queue*)p->q;
	struct list* l = &p->dlist;
	if(q){
		qlock(q);
		list_del(l);
		p->q = 0;
		qunlock(q);	
		return 1;	
	}
	return 0;
}

#endif
