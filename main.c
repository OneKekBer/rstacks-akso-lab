#include "rstack.h"
#include "main.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// TODO: make linter, or some code formatter

//atexit added,

list_t *global_rstacks_list = NULL; // stores all stacks connected to root 

void rstack_list_free_all(){
	list_t *list = global_rstacks_list;
	if(list == NULL) return;
	node_t *current_node = list->head;
	while(current_node != NULL){ 
		node_t *next_node = current_node->next;
		rstack_free(current_node->data.nested_stack);
		free(current_node);
		current_node = next_node;
	}

	free(list);
	global_rstacks_list = NULL;
}

__attribute__((constructor))
static void init(void) {
    atexit(rstack_list_free_all);
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
		if(res == 0)
			rstack_list_free_all();
		
		errno = ENOMEM;
		return NULL;
	}

  new_rstack->is_marked = false;
	new_rstack->front = NULL;
	new_rstack->general_counter = 1;
	new_rstack->inner_counter = 0;

	//i could do this better, TODO: make it in different function
	if(global_rstacks_list == NULL){
		free(new_rstack);
		errno = ENOMEM;
		return NULL;
	}
	
	node_t *new_node = node_new(); // TODO: error handling and change name like in else
	if(new_node == NULL){
		if(res == 0)
			rstack_list_free_all();
			
		free(new_rstack);
		errno = ENOMEM;
		return NULL;
	}

	if(global_rstacks_list->head == NULL){
		new_node->data.nested_stack = new_rstack;

		global_rstacks_list->head = new_node;
		global_rstacks_list->tail = new_node;
	}else{
		new_node->data.nested_stack = new_rstack;
		
		global_rstacks_list->tail->next = new_node;
		global_rstacks_list->tail = new_node; // edited
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

//initialy reset_marks
// bool is_rstack_cycled(rstack_t *rs){
// 	node_t *current_node = rs->front;
// 	rs->is_marked = true; // TODO: explain idea 
	
// 	while(current_node != NULL){ 
// 		if(current_node->type == 1){
// 			rstack_t *nested_stack = current_node->data.nested_stack;
// 			if(nested_stack->is_marked) return true;

// 			bool is_cycle = is_rstack_cycled(nested_stack);
// 			if(is_cycle) return true;
// 		}
		
// 		current_node = current_node->next;
// 	}

// 	rs->is_marked = false;
// 	return false;
// }

void check_is_global_list_empty_and_delete(){
	if(global_rstacks_list == NULL) return;
	if(global_rstacks_list->head == NULL){
		free(global_rstacks_list);
		global_rstacks_list = NULL;
	}
}

typedef enum {
    WRITE_SUCCESS = 0, // Всё отлично
    WRITE_ERROR = 1,   // Критическая ошибка (например, сломался fprintf)
    WRITE_CYCLE = 2    // Обнаружен цикл, нужно штатно прервать запись
} write_status_t;

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

	// Если произошла фатальная ошибка записи (сломался диск и т.д.)
	if (status == WRITE_ERROR) {
		fclose(file);
		remove(path); // Удаляем недописанный мусор
		return -1;
	}

	// Обрати внимание: если status == WRITE_CYCLE или WRITE_SUCCESS, 
	// мы просто идем дальше и штатно закрываем файл, возвращая 0!
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

// is_marked = true for all rs`s child rstacks  
void rstack_mark_childs(rstack_t *rs){
  if(rs == NULL || rs->is_marked) return; 
	// printf("mark child");
  rs->is_marked = true;

  node_t *current_node = rs->front;
  while(current_node != NULL){
    if(current_node->type == 1){ // if node contains stack
			rstack_mark_childs(current_node->data.nested_stack);
    }
		
		current_node = current_node->next;
  }
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

//reset all is_marked to false
void list_reset_marks(list_t *list){
	if(list == NULL){
		errno = EINVAL;
		return;
	}

	node_t *current_node = list->head;
	while(current_node != NULL){
		current_node->data.nested_stack->is_marked = false;
		current_node = current_node->next;
	}
}

//mark is_marked = true for all root rstacks and their child
void list_mark_phase(list_t *list){
	node_t *current_node = list->head;	 

	while(current_node != NULL){
		rstack_t *current_rstack = current_node->data.nested_stack;

		if(current_rstack->general_counter - current_rstack->inner_counter != 0){
			rstack_mark_childs(current_rstack);
			current_rstack->is_marked = true;
		}

		current_node = current_node->next;
	}
}

void rstack_dec_child_counters(node_t *node){
	if(node == NULL) return;
	if(node->type == 1){
		node->data.nested_stack->general_counter--;
		node->data.nested_stack->inner_counter--;
	}

	rstack_dec_child_counters(node->next);
}

void list_sweep_phase_update_counters(list_t *list){
	node_t *list_current_node = list->head;
	
	while (list_current_node != NULL){
		node_t *list_next_node = list_current_node->next;
		
		rstack_t *nested_stack = list_current_node->data.nested_stack;
		if(!nested_stack->is_marked)
			rstack_dec_child_counters(nested_stack->front);
	

		list_current_node = list_next_node;
	}
}

void list_sweep_phase(list_t *list){
	node_t *current_node = list->head;
	node_t *prev_node = NULL; // TODO: assign this node
	node_t *next_node = NULL;

	while(current_node != NULL){
		rstack_t *current_rstack = current_node->data.nested_stack;
		next_node = current_node->next;
		
		if(current_rstack->is_marked){
			prev_node = current_node;
		}
	
		if(!current_rstack->is_marked){
			if(current_node == list->tail){
				list->tail = prev_node;
			}
			if(current_node == list->head){ 
				list->head = list->head->next; // FIXME: maybe this is not enough
			}

			if(prev_node != NULL) prev_node->next = next_node;
			
			rstack_free(current_rstack);
			free(current_node);
		}

		current_node = next_node;
	}
}

void rstack_delete(rstack_t *rs) {
	if (rs == NULL) {
		errno = EINVAL;
		return;
	}

	rs->general_counter--;

	list_reset_marks(global_rstacks_list);
	list_mark_phase(global_rstacks_list);	
	list_sweep_phase_update_counters(global_rstacks_list);
	list_sweep_phase(global_rstacks_list);
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
	
		list_reset_marks(global_rstacks_list);
		list_mark_phase(global_rstacks_list);
		list_sweep_phase_update_counters(global_rstacks_list);
		list_sweep_phase(global_rstacks_list);
		check_is_global_list_empty_and_delete();
	}

	free(front_node);
}

// void test(){
// 	rstack_t *s1 = rstack_new();
// 	rstack_t *s2 = rstack_new();

// 	printf("create");
// 	rstack_push_rstack(s1, s2);
// 	rstack_push_rstack(s1, s2);
// 	rstack_push_rstack(s1, s2);
// 	rstack_push_rstack(s1, s2);
// 	rstack_push_rstack(s1, s2);

// 	//free(s2);
// 	print_list(global_rstacks_list);
// 	print_stack(s1);

// 	printf("%d\n", rstack_empty(s1)); // 1

// 	rstack_push_value(s2, 5);
// 	printf("%d", rstack_empty(s1)); // 0
// 	printf("\n");
// 	printf("%d", rstack_empty(s2)); // 0

// }

// int main() {
// 	// test();
// 	printf("main");
// 	rstack_t *read = rstack_read("c/file_four.in"); 
// 	// print_stack(read);
// 	rstack_write("c/answer.txt", read); 
	
// 	return 0;
// }

