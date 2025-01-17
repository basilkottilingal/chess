#include "move.h"

typedef struct _GameTreeEdge{
  _Game * g;
  _GameMove * m;
  //depth
  unsigned char depth; // [0,8) ?? 
  //Graph Connection using pointers
  struct _GameTreeEdge * prev, * next;
} GameTreeEdge;

typedef struct {
  _GameTreeEdge * root;
} _GameTree;

