#include "move.h"

typedef struct _GameEdge{
  _Game * g;
  _GameMove * m;
  //depth
  unsigned char depth; // [0,8) ?? 
  //Graph Connection using pointers
  struct _GameEdge * parent, ** children;
} GameEdge;

typedef struct {
  _GameEdge * root;
} _GameTree;

void GameEdgeCreate()

_GameTree * GameTree(_Game * current) {
  _GameTree * t = 
    (_GameTree *) malloc (sizeof(_GameTree));
  _GameTree * root = 
    (_GameEdge *) malloc (sizeof(_GameEdge));
  _Game * g = GameCopy(current);

  root->g = g;
  root->parent = NULL;
  int nmoves = (int ) (g->moves->len) / sizeof (_GameMove);
  root->children = !nmoves ? NULL :
    ((_GameEdge **) malloc (nmoves sizeof(_GameEdge *)));

  t->root = root; 
}
