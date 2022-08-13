#ifndef __QUEUE_H
#define __QUEUE_H

#include "types.h"
#include "riscv.h"
#include "proc.h"
#include "printf.h"
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
	return list_empty(&q->head);
}

static inline void queue_push(queue* q,struct proc* p){
	qlock(q);
	list_add_before(&q->head,&p->dlist);
	p->q = (uint64)q;
	qunlock(q);
}

static inline struct proc* queue_pop(queue* q){
	struct proc* p = NULL;
	qlock(q);
	if(!queue_empty(q)){
		struct list* l = list_next(&q->head);
		list_del(l);
		p = dlist_entry(l, struct proc, dlist);
		p->q = 0;
		p->dlist.prev = NULL;
		p->dlist.next = NULL;
	}
	qunlock(q);
	return p;
}

static inline int queue_del(struct proc* p){
	queue* q = (queue*)p->q;
	struct list* l = &p->dlist;
	qlock(q);
	if(q){
		list_del(l);
		p->q = 0;
		p->dlist.prev = NULL;
		p->dlist.next = NULL;
		qunlock(q);	
		return 1;	
	}
	qunlock(q);
	return 0;
}

#endif
