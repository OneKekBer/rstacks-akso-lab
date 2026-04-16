#include "garbage_collector.h"
#include "main.h"
#include "rstack.h"

#include <errno.h>
#include <stdlib.h>

void rstack_mark_childs(rstack_t *rs) {
	if (rs == NULL || rs->is_marked)
		return;
	rs->is_marked = true;

	node_t *current_node = rs->front;
	while (current_node != NULL) {
		if (isstack(current_node))
			rstack_mark_childs(current_node->nested_stack);

		current_node = current_node->next;
	}
}

list_node_t *list_node_new(rstack_t *rstack) {
	list_node_t *node = (list_node_t *)malloc(sizeof(list_node_t));
	if (node == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	node->rstack = rstack;
	node->next = NULL;

	return node;
}

// reset all is_marked to false
void list_reset_marks(list_t *list) {
	if (list == NULL) {
		errno = EINVAL;
		return;
	}

	list_node_t *current_node = list->head;
	while (current_node != NULL) {
		current_node->rstack->is_marked = false;
		current_node = current_node->next;
	}
}

// mark is_marked = true for all root rstacks and their child
void list_mark_phase(list_t *list) {
	list_node_t *current_node = list->head;

	while (current_node != NULL) {
		rstack_t *current_rstack = current_node->rstack;

		if (current_rstack->general_counter - current_rstack->inner_counter != 0) {
			rstack_mark_childs(current_rstack);
			current_rstack->is_marked = true;
		}

		current_node = current_node->next;
	}
}

void rstack_dec_child_counters(node_t *node) {
	if (node == NULL)
		return;

	if (isstack(node)) {
		node->nested_stack->general_counter--;
		node->nested_stack->inner_counter--;
	}

	rstack_dec_child_counters(node->next);
}

void list_sweep_phase_update_counters(list_t *list) {
	list_node_t *list_current_node = list->head;

	while (list_current_node != NULL) {
		list_node_t *list_next_node = list_current_node->next;

		rstack_t *nested_stack = list_current_node->rstack;
		if (!nested_stack->is_marked)
			rstack_dec_child_counters(nested_stack->front);

		list_current_node = list_next_node;
	}
}

void list_sweep_phase(list_t *list) {
	list_node_t *current_node = list->head;
	list_node_t *prev_node = NULL;
	list_node_t *next_node = NULL;

	while (current_node != NULL) {
		rstack_t *current_rstack = current_node->rstack;
		next_node = current_node->next;

		if (current_rstack->is_marked) {
			prev_node = current_node;
		}

		if (!current_rstack->is_marked) {
			if (current_node == list->tail) {
				list->tail = prev_node;
			}
			if (current_node == list->head) {
				list->head = list->head->next;
			}

			if (prev_node != NULL)
				prev_node->next = next_node;

			rstack_free(current_rstack);
			free(current_node);
		}

		current_node = next_node;
	}
}

void clean(list_t *list) {
	list_reset_marks(list);
	list_mark_phase(list);
	list_sweep_phase_update_counters(list);
	list_sweep_phase(list);
	check_is_global_list_empty_and_delete();
}