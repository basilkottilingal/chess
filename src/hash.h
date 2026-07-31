#ifndef _CHESS_HASH_H_
#define _CHESS_HASH_H_
  #include "engine.h"
  #include "mempool.h"
  
  #define TABLE_SIZE (1<<16)
  #ifndef HASH_BLOCKS
  /* One block = (1<<16) * sizeof(_Hash) = 1MB */
  #define HASH_BLOCKS 2
  #endif
  
  typedef struct
  {
    uint64_t key;    
    uint32_t security; 
    short score;  
    Flag depth, flags;
  } _Hash;
  
  _Hash ** HASH_TABLE = NULL;
  static uint64_t HASH_MASK = 0;
  Flag HashBlock[8];
  
  Flag HashInit ()
  {
    uint16_t nb = n = HASH_BLOCKS, r = 0, N = 0;
    while (n)
    {
      if (r)
        break;
      r = n & 1;  /* reminder */
      n = n >> 1; /* n = n/2  */
      ++N;
    }

    if ( (!N) || (N>5) || (n && r) )
    { 
      GameError("HASH_BLOCKS should be in [1,2,4,8,16]");
      return GAME_ERROR;
    }
  
    HASH_POOL = Mempool (sizeof(_Hash), 1 + 1024*1024/sizeof(_Hash)); 
  
    HashTable = malloc ((N+1)* sizeof(_Hash *));
    for (int i=0; i <= N; ++i)
    {
      _Mem
      HashTable[i] = (_Hash *)
    }
  
    return GAME_CONTINUE;
  }
  
  
  uint64_t H_TABLE[64][12], H_CASTLE[16], 
    H_ENP[8], H_COLOR;
  
  static inline 
  _Hash * Insert(_Board * b, Flag depth, Flag flags) {
    /* See if board is already stored in hash table 
    .. with depth >= depth or 
    .. insert. Return evaluation of board*/
    if(depth >  
  }
  
#endif
