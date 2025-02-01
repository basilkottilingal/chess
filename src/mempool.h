#include <stdio.h>
#include <stdlib.h>

// Define a struct for the free list node
typedef struct FreeNode {
    struct FreeNode *next; // Pointer to the next free slot
} FreeNode;

// Define the memory pool struct
typedef struct {
    size_t object_size;  // Size of each object
    size_t block_size;   // Number of objects in the block
    void *memory_block;  // Pointer to the memory block
    FreeNode *free_list; // Linked list of free slots
} MemoryPool;

// Initialize the memory pool
MemoryPool* init_memory_pool(size_t object_size, size_t block_size) {
    MemoryPool *pool = (MemoryPool*)malloc(sizeof(MemoryPool));
    if (!pool) {
        fprintf(stderr, "Failed to allocate memory for the pool.\n");
        return NULL;
    }

    pool->object_size = object_size;
    pool->block_size = block_size;
    pool->memory_block = malloc(object_size * block_size);
    if (!pool->memory_block) {
        fprintf(stderr, "Failed to allocate memory block.\n");
        free(pool);
        return NULL;
    }

    // Initialize the free list
    pool->free_list = NULL;
    for (size_t i = 0; i < block_size; i++) {
        FreeNode *node = (FreeNode*)((char*)pool->memory_block + i * object_size);
        node->next = pool->free_list;
        pool->free_list = node;
    }

    return pool;
}

// Allocate memory from the pool
void* allocate_from_pool(MemoryPool *pool) {
    if (!pool || !pool->free_list) {
        fprintf(stderr, "No free slots available in the pool.\n");
        return NULL;
    }

    // Remove the first node from the free list
    FreeNode *allocated_node = pool->free_list;
    pool->free_list = allocated_node->next;

    return (void*)allocated_node;
}

// Deallocate memory back to the pool
void deallocate_to_pool(MemoryPool *pool, void *ptr) {
    if (!pool || !ptr) return;

    // Add the slot back to the free list
    FreeNode *node = (FreeNode*)ptr;
    node->next = pool->free_list;
    pool->free_list = node;
}

// Free the entire memory pool
void free_memory_pool(MemoryPool *pool) {
    if (!pool) return;

    free(pool->memory_block);
    free(pool);
}

// Example usage: Create a linked list
typedef struct Node {
    int data;
    struct Node *next;
} Node;

int main() {
    // Initialize a memory pool for Node objects
    size_t num_objects = 10;
    MemoryPool *node_pool = init_memory_pool(sizeof(Node), num_objects);
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
