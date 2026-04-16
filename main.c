#include "rstack.h"
#include "garbage_collector.h"
#include "main.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static list_t *global_rstacks_list = NULL;

// TODO: make linter, or some code formatter

__attribute__((constructor))
static void init(void) {
  atexit(rstack_list_free_all);
}

void check_is_global_list_empty_and_delete(){
	if(global_rstacks_list == NULL) return;
	if(global_rstacks_list->head == NULL){
		free(global_rstacks_list);
		global_rstacks_list = NULL;
	}
}

int init_global_rstacks_list() {
	if(global_rstacks_list != NULL) return -1;
	global_rstacks_list = (list_t*)malloc((size_t)sizeof(list_t));

	if(global_rstacks_list == NULL) { // TOTAL UNLUCK
		errno = ENOMEM;
		return -1;
	}

	global_rstacks_list->head = NULL;
	global_rstacks_list->tail = NULL;
	
	return 0;
}

node_t *node_new() {
	node_t *node = (node_t *)malloc((size_t)sizeof(node_t));
	if (node == NULL) {
		errno = ENOMEM;
		return NULL;
	}

	node->data.nested_stack = NULL;
	node->data.value = 0;
	node->next = NULL; 
	node->type = 0;

	return node;
}

rstack_t *rstack_new() {
	int res = init_global_rstacks_list();
	
	rstack_t *new_rstack = (rstack_t *)malloc(sizeof(rstack_t));
	if (new_rstack == NULL) {
		if(res == 0) check_is_global_list_empty_and_delete();
		errno = ENOMEM;
		return NULL;
	}

	new_rstack->is_marked = false;
	new_rstack->front = NULL;
	new_rstack->general_counter = 1;
	new_rstack->inner_counter = 0;

	if(global_rstacks_list == NULL){
		free(new_rstack);
		errno = ENOMEM;
		return NULL;
	}
	
	list_node_t *new_list_node = list_node_new(new_rstack);
	if(new_list_node == NULL){
		if(res == 0) check_is_global_list_empty_and_delete();
		free(new_rstack);
		errno = ENOMEM;
		return NULL;
	}

	if(global_rstacks_list->head == NULL){
		global_rstacks_list->head = new_list_node;
		global_rstacks_list->tail = new_list_node;
	} else {
		global_rstacks_list->tail->next = new_list_node;
		global_rstacks_list->tail = new_list_node;
	}

	return new_rstack;
}

void rstack_read_fail(rstack_t *rs, FILE *f, char *buffer){
	if(rs != NULL) rstack_delete(rs);
	fclose(f);
	if(buffer != NULL) free(buffer);
}

rstack_t *rstack_read(char const *path) {
	//TODO: is fopen through errno if path is unvavaible
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		errno = ENOMEM; // TODO: check blyat
		return NULL;
	}

	//init rstack and check on allocation 
	rstack_t *current_rstack = rstack_new(); 
	if(current_rstack == NULL){
		rstack_read_fail(NULL, file, NULL);
		errno = ENOMEM;
		return NULL;
	}

	char *buffer = NULL;
	size_t size = 0;

	while (getline(&buffer, &size, file) != -1){
		char *number_end = NULL;
		char *number_start = buffer;
		
		uint64_t input_number = 0;
		while(true){
			errno = 0;
			input_number = strtoull(number_start, &number_end, 10);
			if (errno == ERANGE){
				rstack_read_fail(current_rstack, file, buffer);
				return NULL;
			}

			if(number_end == number_start) {
				while(isspace(*number_start)) number_start++;

				if(*number_start == '\n' || *number_start == '\0' || isspace(*number_start)) break; // TODO: wyjebac isspace
				rstack_read_fail(current_rstack, file, buffer);
				errno = EINVAL;
				return NULL;
			}

			int is_fail = rstack_push_value(current_rstack, (uint64_t)(input_number));
			if(is_fail){
				rstack_read_fail(current_rstack, file, buffer);
				return NULL;
			}
						
			number_start = number_end;
		}
	}
	
	free(buffer);
	fclose(file);
	return current_rstack;
}

write_status_t write_single_node_rec(node_t *node, FILE *file) {
	if (node == NULL) return WRITE_SUCCESS;

	write_status_t status = write_single_node_rec(node->next, file);
	if (status != WRITE_SUCCESS) return status;

	if (node->type == 1) {
		rstack_t *nested_stack = node->data.nested_stack;
		if (nested_stack->is_marked) return WRITE_CYCLE;
		
		nested_stack->is_marked = true;
		status = write_single_node_rec(nested_stack->front, file);
		nested_stack->is_marked = false;
		
		if (status != WRITE_SUCCESS) return status;
	}

	if (node->type == 0) {
		if (fprintf(file, "%" PRIu64 "\n", node->data.value) < 0) { // TODO: check if buffered and whether it can cause problems
			return WRITE_ERROR;
		}
	}

	return WRITE_SUCCESS;
}

int rstack_write(char const *path, rstack_t *rs) {
	if (rs == NULL) {
		errno = EINVAL;
		return -1;
	}

	list_reset_marks(global_rstacks_list);

	FILE *file = fopen(path, "w");
	if (file == NULL) {
		return -1;
	}
	
	rs->is_marked = true;
	write_status_t status = write_single_node_rec(rs->front, file);

	if (status == WRITE_ERROR) {
		fclose(file);
		remove(path); 
		return -1;
	}

	if (fclose(file) != 0) {
			return -1;
	}

	return 0;
}

void find_is_rstack_empty(node_t *node, bool *is_empty){
	if(node == NULL) return;

	if(node->type == 0){
		(*is_empty) = false;
		return;
	}
	
	rstack_t *child_stack = node->data.nested_stack;
	if(!child_stack->is_marked){
		child_stack->is_marked = true;
		find_is_rstack_empty(child_stack->front, is_empty);
	} 

	if(!(*is_empty)) return;
	find_is_rstack_empty(node->next, is_empty);
}

bool rstack_empty(rstack_t *rs){
	if(rs == NULL){
		errno = EINVAL;
		return true;
	}
	
	list_reset_marks(global_rstacks_list); //  use is_marked list like is visited
	rs->is_marked = true;

	bool is_empty = true;
	find_is_rstack_empty(rs->front, &is_empty);
	return is_empty;
}

result_t result_new(bool is_success, uint64_t value){
	result_t result;

	result.flag = is_success;
	result.value = value;

	return result;
}

result_t find_nearest_value(node_t *node){
	if(node == NULL) return result_new(false, 0);
	
	if(node->type == 0) return result_new(true, node->data.value);
	
	rstack_t *nested_stack = node->data.nested_stack;
	
	result_t child_result = result_new(false, 0);
	if(!nested_stack->is_marked){
		nested_stack->is_marked = true;
		child_result = find_nearest_value(nested_stack->front); 
	}

	if(!child_result.flag){
		return find_nearest_value(node->next);
	}

	return child_result; 
}

result_t rstack_front(rstack_t *rs){
	if(rs == NULL){
		errno = EINVAL; // this einval can be deleted by null result 
		return result_new(false, 0); 
	}

	list_reset_marks(global_rstacks_list);
	rs->is_marked = true;

	result_t front_value_result = find_nearest_value(rs->front); // how to handle stack overflow
	if(!front_value_result.flag){
		return result_new(false, 0);
	}	

	return result_new(true, front_value_result.value);
}

int rstack_push_rstack(rstack_t *rs1, rstack_t *rs2) {
	if (rs1 == NULL || rs2 == NULL) {
		errno = EINVAL;
		return -1;
	}

	node_t *prev_front = rs1->front;

	node_t *new_stack_node = node_new();
	if(new_stack_node == NULL){
		return -1;
	}

	rs2->general_counter++;
	rs2->inner_counter++;

	new_stack_node->data.nested_stack = rs2;
	new_stack_node->type = 1;
	new_stack_node->next = prev_front;

	rs1->front = new_stack_node;

	return 0;
}

// return -1 if fails, 0 if is success
int rstack_push_value(rstack_t *rs, uint64_t value) {
	if (rs == NULL) {
		errno = EINVAL;
		return -1;
	}
	node_t *value_node = node_new();
	if(value_node == NULL){
		return -1;
	}
	value_node->type = 0;
	value_node->data.value = value;

	node_t *prev_front = rs->front;
	value_node->next = prev_front;
	rs->front = value_node;

	return 0;
}

//delete all rstack value nodes 
void rstack_free(rstack_t *rs){
	node_t *current_node = rs->front;
	node_t *next_node = NULL;

	while(current_node != NULL){
		next_node = current_node->next;	
		free(current_node);
		current_node = next_node;
	}

	free(rs);
}

void rstack_delete(rstack_t *rs) {
	if (rs == NULL) {
		errno = EINVAL;
		return;
	}

	rs->general_counter--;

	clean(global_rstacks_list);
	check_is_global_list_empty_and_delete();
}

void rstack_pop(rstack_t *rs) {
	if (rs == NULL) {
		errno = EINVAL;
		return;
	}

	node_t *front_node = rs->front;
	if (front_node == NULL) {
		return;
	}

	node_t *next_front_node = front_node->next; // if front->next == NULL its doesnt matter
	rs->front = next_front_node;

	if (front_node->type == 1) {
		rstack_t *front_rstack = front_node->data.nested_stack;
		
		front_rstack->general_counter--;
		front_rstack->inner_counter--;
		
		clean(global_rstacks_list);
		check_is_global_list_empty_and_delete();
	}

	free(front_node);
}