#include <stdio.h>
#include <stdlib.h>

#define MEMPOOL 

// Define a struct for the free list node
typedef struct _FreeNode {
  struct _FreeNode *next; // Pointer to the next free slot
}_FreeNode;

// Define the memory pool struct
typedef struct {
  size_t object_size;  // Size of each object
  size_t block_size;   // Number of objects in the block
  void * memory_block;  // Pointer to the memory block
  /* NOTE: Perfer making blocks that align with ..
  .. multiples of kB or MB */
  _FreeNode * free_list; // Linked list of free slots
} _MemPool;

_MemPool* MemPool(size_t object_size, size_t block_size) {
  // Initialize the memory pool
  // FIXME: Redo this:

  if(object_size < sizeof (void *)){
    fprintf(stderr, "\nERROR: \
Oject size should be atleast %ld ", sizeof(void *));
    // Otherwise you might overwrite when typecasting ..
    // .. empty memspaces to _FreeNode * 
    fflush(stderr);  
    return NULL;
  }
    
  _MemPool * pool = 
    (_MemPool*)malloc(sizeof(_MemPool));
  if (!pool) {
    fprintf(stderr, "\nERROR: \
Failed to allocate memory for the pool.\n");
    fflush(stderr);  
    return NULL;
  }

  pool->object_size  = object_size;
  pool->block_size   = block_size;
  pool->memory_block = malloc(object_size * block_size);
  if (!pool->memory_block) {
    fprintf(stderr, "\nERROR:\
Failed to allocate memory block for the pool.");
    free(pool);
    return NULL;
  }

  // Initialize the free list
  pool->free_list = NULL;
  char * block = (char *)  pool->memory_block;
  for (size_t i = 0; i < block_size; i++,block+= object_size) {
    _FreeNode * node = (_FreeNode*) block;
    node->next = pool->free_list;
    pool->free_list = node;
  }

  return pool;
}

// Allocate memory from the pool
void * MemPoolAllocateFrom(_MemPool * pool) {
  if (!pool){
    fprintf(stderr, "ERROR: No pool Mentioned!");
    fflush(stderr);
    return NULL;
  }
  if (!pool->free_list) {
    fprintf(stderr, "WARNING:\
No free slots available in the pool.");
    fflush(stderr);
    return NULL;
  }

  // Remove the first node from the free list
  _FreeNode *allocated_node = pool->free_list;
  pool->free_list = allocated_node->next;
  return (void*)allocated_node;
}

// Deallocate memory back to the pool
void MemPoolDeallocateTo(_MemPool * pool, void * ptr) {
  if (!pool || !ptr) {
    fprintf(stderr, "ERROR:\
Either of pool or node NOT mentioned!");
    fflush(stderr);
    return;
  }

  // Add the slot back to the free list
  _FreeNode *node = (_FreeNode*)ptr;
  node->next = pool->free_list;
  pool->free_list = node;
}

// Free the entire memory pool
void MemPoolFree(_MemPool *pool) {
  if (!pool) return;

  free(pool->memory_block);
  free(pool);
}


/*
// Example usage: Create a linked list
typedef struct Node {
  int data;
  struct Node *next;
} Node;
int main() {
    // Initialize a memory pool for Node objects
    size_t num_objects = 10;
    MemPool *node_pool = init_memory_pool(sizeof(Node), num_objects);
    if (!node_pool) return 1;

    // Allocate a few nodes
    Node *node1 = (Node*)allocate_from_pool(node_pool);
    Node *node2 = (Node*)allocate_from_pool(node_pool);
    Node *node3 = (Node*)allocate_from_pool(node_pool);

    // Set data and link the nodes
    node1->data = 1;
    node2->data = 2;
    node3->data = 3;
    node1->next = node2;
    node2->next = node3;
    node3->next = NULL;

    // Print the linked list
    Node *current = node1;
    while (current) {
        printf("Node data: %d\n", current->data);
        current = current->next;
    }

    // Deallocate nodes back to the pool
    deallocate_to_pool(node_pool, node1);
    deallocate_to_pool(node_pool, node2);
    deallocate_to_pool(node_pool, node3);

    // Free the memory pool
    free_memory_pool(node_pool);

    return 0;
}
*/
