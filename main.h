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

typedef enum {
    WRITE_SUCCESS = 0, 
    WRITE_ERROR = 1,   
    WRITE_CYCLE = 2
} write_status_t;

typedef struct node {
	int type; // 0 - number, 1 - stack

	union {
		uint64_t value;
		struct rstack *nested_stack;
	} data;

	struct node *next; // next node in current stack
} node_t;

#endif