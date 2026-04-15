#include "rstack.h"
#include "main.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

void print_list(list_t *list) {
	if (list == NULL) {
		printf("list = NULL\n");
		return;
	}

	node_t *current = list->head;
	int index = 0;

	printf("\n=== GLOBAL RSTACK LIST ===\n");

	while (current != NULL) {
		rstack_t *rs = current->data.nested_stack;

		printf("Node %d:\n", index);
		// printf("  addr node        = %p\n", (void*)current);
		// printf("  addr rstack      = %p\n", (void*)rs);
		printf("  general_counter  = %d\n", rs->general_counter);
		printf("  inner_counter    = %d\n", rs->inner_counter);
		printf("  is_marked        = %d\n", rs->is_marked);
		printf("\n");

		current = current->next;
		index++;
	}

	// printf("HEAD = %p\n", (void*)list->head);
	// printf("TAIL = %p\n", (void*)list->tail);
	// printf("===========================\n\n");
}

void print_stack(rstack_t *rs) {
	if (rs == NULL) {
		printf("stack = NULL\n");
		return;
	}

	printf("\n=== STACK ===\n");
	printf("general_counter = %d\n", rs->general_counter);
	printf("inner_counter   = %d\n", rs->inner_counter);
	printf("is_marked       = %d\n", rs->is_marked);
	printf("-----------------\n");

	node_t *current = rs->front;
	int index = 0;

	while (current != NULL) {
		printf("Node %d: ", index);

		if (current->type == 0) {
			// value
      printf("VALUE = %" PRIu64 "\n", current->data.value);
		} else if (current->type == 1) {
			// nested stack
			printf("STACK -> %p\n", (void*)current->data.nested_stack);
		} else {
			printf("UNKNOWN TYPE\n");
		}

		current = current->next;
		index++;
	}

	printf("=================\n\n");
}