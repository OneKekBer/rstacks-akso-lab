#ifndef _GARBAGE_COLLECTOR_H
#define _GARBAGE_COLLECTOR_H

#include "main.h"

typedef struct list_node {
	rstack_t *rstack;
	struct list_node *next;
} list_node_t;

typedef struct list {
	list_node_t *head; // maybe i could create custom nodes, but its good with that nodes
	list_node_t *tail;
} list_t;

list_node_t *list_node_new(rstack_t *rstack);
void rstack_list_free_all(list_t *list);
void list_reset_marks(list_t *list);
void clean(list_t *list);

#endif