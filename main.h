#ifndef _MAIN_H
#define _MAIN_H

#include "rstack.h"

typedef struct node node_t;

struct rstack {
	node_t *front;
	int general_counter;
	int inner_counter;
	bool is_marked; // for garbage collector algo (mark and sweep)
};

// node can store value or stack
typedef struct node {
	uint64_t value;
	struct rstack *nested_stack;
	struct node *next; // next node in current stack
} node_t;

int isvalue(node_t *node);
int isstack(node_t *node);
void check_is_global_list_empty_and_delete();
void rstack_free(rstack_t *rstack);

#endif