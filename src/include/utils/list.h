#ifndef __LIST_H
#define __LIST_H

#include "include/types.h"
#include "include/printf.h"

struct list {
	struct list *prev, *next;
};

#define	dlist_entry(ptr, type, member) \
				container_of(ptr, type, member)


/**
 * Auxiliary method. Don't recommend to use directly.
 */

static inline void __list_insert(struct list *node, struct list *prev, struct list *next) {
	prev->next = next->prev = node;
	node->next = next;
	node->prev = prev;
}

static inline void __list_link(struct list *prev, struct list *next) {
	prev->next = next;
	next->prev = prev;
}


/**
 * Simple functions. We don't check legality inside them.
 */

static inline void list_init(struct list *node) {
	node->prev = node->next = node;
}

static inline int list_empty(struct list *head) {
	return head->next == head;
}

static inline void list_add_after(struct list *afterme, struct list *node) {
	__list_insert(node, afterme, afterme->next);
}

static inline void list_add_before(struct list *beforeme, struct list *node) {
	__list_insert(node, beforeme->prev, beforeme);
}

static inline void list_del(struct list *node) {
	if(node==NULL){
	  panic("[list next]list is null");
	}
	__list_link(node->prev, node->next);
}

static inline struct list *list_prev(struct list *node) {
	return node->prev;
}

static inline struct list *list_next(struct list *node) {
	return node->next;
}


#endif
