#include <stdio.h>
#include <stdlib.h>

#define MEMPOOL 

#define SAFETY_ENCODE 0xFBC9183 

/* Define a struct for the free list node */
typedef struct _FreeNode {
  /* Pointer to the next free slot */
  struct _FreeNode *next; 

  /* while freeing, encode "safety" to SAFETY_ENCODE 
  .. to avoid double freeing of pool
  */
  uint32_t safety;
}_FreeNode;

/* Memory pool */
typedef struct _Mempool {

  /* Size of a node */
  size_t object_size;  

  /* memory block size */
  size_t block_size;  
 
  /* No of freenodes in the block */
  size_t nfree;   

  /* Pointer to the memory block .
  .. NOTE: Perfer making blocks that align with ..
  .. multiples of kB or MB */
  void * memory_block; 
 
  /* Staring node of  Linked list of free slots */
  _FreeNode * free_list; 

  /* incase of a linked list of pool is reqd
  struct _Mempool * next; 
  */
} _Mempool;

/* Creating a memor pool*/
_Mempool * Mempool(size_t object_size, size_t nobjects) {

  if(object_size < sizeof (_FreeNode)){
    /* Otherwise you might overwrite used nodes when 
    .. typecasting empty memspaces to _FreeNode */
    GameError("Mempool() : insufficient object_size");
    return NULL;
  }
  if(object_size%8) 
    GameError ("Mempool() : Warning: Ignoring 64 bit alignment criteria");
    
  _Mempool * pool = 
    (_Mempool*)malloc(sizeof(_Mempool));

  size_t Bytes = nobjects * object_size,
    kB = Bytes/1024, MB = Bytes/(1024*1024);
  
  //Rounding to closer kB , or MB
  size_t block_size = kB < 1024 ? //less than an MB?
    (kB+1)*1024 :  1024*1024; //1 MB is the limit
  
  size_t nfree = block_size/object_size;
  
  if(MB > 1) 
    //1MB is the limit
    fprintf(stderr, "\nWARNING: Block size rounded to 1MB");

  pool->object_size  = object_size;
  pool->block_size   = block_size;
  pool->nfree        = nfree;
  pool->memory_block = malloc(block_size);
  if (!pool->memory_block) {
    GameError("Mempool() : malloc() of memory block failed");
    free(pool);
    return NULL;
  }

  // Initialize the free list
  pool->free_list = NULL;
  char * block = (char *)  pool->memory_block;
  for (size_t i = 0; i < nfree; i++,block+= object_size) {
    _FreeNode * node = (_FreeNode*) block;
    node->next = pool->free_list;
    node->safety = SAFETY_ENCODE;
    pool->free_list = node;
  }

  fprintf(stdout, "\nMemBlock[%ld bytes] created. \n\
Can accomodate %ld Objects each of size %ld\n",
pool->block_size, pool->nfree, pool->object_size);
  fflush(stdout);

  return pool;
}

// Allocate memory from the pool
void * MempoolAllocateFrom(_Mempool * pool) {
  if (!pool){
    GameError("MempoolAllocateFrom() : No pool Mentioned!");
    return NULL;
  }
  if (!pool->free_list) {
#if 0
    fprintf(stderr, 
      "WARNING : No free slots available in the pool.");
    fflush(stderr);
#endif
    return NULL;
  }

  // Remove the first node from the free list
  _FreeNode * node = pool->free_list;
  pool->free_list = node->next;
  --(pool->nfree);
  if(node->safety != SAFETY_ENCODE)  {
    GameError("MempoolAllocateFrom() : double allocation!");
    return NULL;
  }
  node->safety = 0;

  return (void*) node;
}

/* Deallocate memory back to the pool */
Flag MempoolDeallocateTo(_Mempool * pool, void * ptr) {
  if (!pool || !ptr) {
    GameError("MempoolDeallocateTo() : aborted");
    return 0;
  }

  /* Add the slot back to the free list.
  .. Check if it's a double freeing */
  _FreeNode *node = (_FreeNode*)ptr;
  if(node->safety == SAFETY_ENCODE)  {
    GameError("MempoolDeallocateTo() : double freeing");
    return 0;
  }
  node->next = pool->free_list;
  node->safety = SAFETY_ENCODE;
  pool->free_list = node;
  ++(pool->nfree);

  return 1;
}

/* Free the entire memory pool */
void MempoolFree(_Mempool *pool) {
  if (!pool) return;

  free(pool->memory_block);
  free(pool);
}

