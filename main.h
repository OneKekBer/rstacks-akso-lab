#include "rstack.h"

typedef struct node node_t;

struct rstack {
	node_t *front;
	int general_counter;
	int inner_counter;
	bool is_marked; // for garbage collector algo (mark and sweep)
};

typedef struct node {
	int type; // 0 - number, 1 - stack

	union {
		uint64_t value;
		struct rstack *nested_stack;
	} data;

	struct node *next; // next node in current stack
} node_t;

typedef struct list {
	node_t *head; // maybe i could create custom nodes, but its good with that nodes
	node_t *tail;
} list_t;

void print_list(list_t *list);
void print_stack(rstack_t *rs);