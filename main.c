#include <stdlib.h>
#include "rstack.h"
#include <errno.h>
#include <stdio.h>
//Każde odłożenie stosu na stos zwiększa o jeden wartość licznika referencji odkładanego stosu
//Każde zdjęcie stosu ze stosu zmniejsza o jeden wartość licznika referencji zdejmowanego stosu
//Kasowanie stosu polega na zmniejszeniu o jeden wartości licznika referencji 

//TODO make linter, or some code formatter
typedef struct node node_t;

struct rstack{
  node_t *front;
  int counter;
};

typedef struct node {
  int type; // 0 - number, 1 - stack

  union {
    uint64_t value;
    struct rstack *nested_stack; 
  } data;

  struct node *next; //next node in current stack
} node_t;

rstack_t* rstack_read(char const *path){
  auto file = fopen(path, "r");
  if(file == NULL){
    errno = ENOENT;// No such file or directory
    return NULL;
  }

  return NULL;
}

rstack_t* rstack_new(){
  rstack_t *rs = (rstack_t*)malloc(sizeof(rstack_t));
  if(rs == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  rs->front = NULL;
  rs->counter = 1;

  return rs;
}

node_t* node_new(){
  node_t* node = (node_t*)malloc((size_t)sizeof(node_t));
  if(node == NULL){
    errno = ENOMEM;
    return NULL;  
  }

  return node;
}

int rstack_push_rstack(rstack_t *rs1, rstack_t *rs2){
  if(rs1 == NULL || rs2 == NULL){
    errno = EINVAL;
    return -1;
  }

  if(rs2->counter == 0 || rs1->counter == 0){ //mb useless
    errno = EINVAL;
    return -1;
  }

  rs2->counter++;
  node_t *prev_front = rs1->front;
  
  node_t *new_stack_node = node_new();
  new_stack_node->data.nested_stack = rs2;
  new_stack_node->type = 1;
  new_stack_node->next = prev_front;
  
  rs1->front = new_stack_node;
  
  return 0;
}

int rstack_push_value(rstack_t *rs, uint64_t value){
  if(rs == NULL) {
    errno = EINVAL;
    return -1;
  }
  node_t* value_node = node_new();
  
  value_node->type = 0;
  value_node->data.value = value; 

  auto prev_front = rs->front;  
  value_node->next = prev_front;  
  rs->front = value_node;

  return 0; 
}
// maybe its good to know in what stack rs stores
// maybe i should have some array where key is stack id, value id of stack where it contains

// 2 rstack_delete functions ?? one default will delete main connection, second will delete internal connection
// 

// I assume that user cant call this function. 
// If its not true, all stack structure will collapse 
void rstack_delete(rstack_t *rs){
  if(rs == NULL){
    errno = EINVAL;
  }

  rs->counter--;
  if(rs->counter == 0){
    // what if this stack stores another stack??
    // -- will i lost my references to child stacks ?? 
    // - i should iterate through all child stack and make for them rstack_delete --DONE
    // - also free all value nodes --DONE
    
    node_t *current_node = rs->front; 
    while(current_node != NULL){  // maybe i can do this in better way
      if(current_node->type == 0){ // value node
        node_t *next = current_node->next;
        free(current_node);
        current_node = next;
      }else{ // stack node
        rstack_delete(current_node->data.nested_stack); 
        node_t *next = current_node->next;
        free(current_node);
        current_node = next;
      }
    } 

    free(rs);
    return;
  }
  // if rs->counter != 0
  // - check on cycled bullshit
  // -- need to find is "graph" is cycled 
  // -- maybe i need to differ connections, like root and
  //      other stack (but in rstack_delete i dont choose which connection i need to delete)
  //  
  // assume i can differ connection
  // i have cycled stacks, if i will iterate through(precisely through every node) them
  // if any of them wont have root connection -> free everything. bfs? 

  return;
}

void rstack_pop(rstack_t *rs){
  if(rs == NULL){
    errno = EINVAL;
    return;
  }

  node_t *front = rs->front;
  if(front == NULL){
    //i think there is no need to errno
    return;
  } 

  if(front->type == 1){
    rstack_delete(front->data.nested_stack);
  }
  
  free(front); // im not sure do i need to free it
  node_t *next_node = front->next; // if front->next == NULL its doesnt matter
  rs->front = next_node;
}



int main(){
  //rstack_push_value(rs1, 5);

  return 0;
}
