#include "move.h"

_Game * GameCopy(_Game * _g) {
  //Game instance 'g' copied from '_g'
  _Game * g = (_Game *) malloc (sizeof (_Game));

  int b = 8, p = 2;
  int tb = b + 2*p;
  _GameSquare ** board = (_GameSquare **)
    malloc(tb*sizeof(_GameSquare *));
  board[0] = (_GameSquare *)
    malloc(tb*tb*sizeof(_GameSquare));

  //board is copied
  memcpy(board[0], _g->board[0], tb*tb*sizeof(_GameSquare) );

  for(int i=1; i<tb; ++i)
    board[i] = board[i-1] + tb;
  for(int i=0; i<tb; ++i) {
    for(int j=0; j<tb; ++j)
      board[i][j].piece = OUTSIDE; 
    board[i] += p;
  }
  board += p;
  g->board = board;

  g->move  = NULL;
  g->history = NULL; //Not used.

  // Copy moves from "_g->moves" rather than generating moves
  g->moves = array_new();
  array_append(g->moves, _g->moves->p, _g->moves->len);
  // shrink array to avoid using larger memory for ..
  // .. smaller sized data
  array_shrink(g->moves);
  g->check  = _g->check;
  g->enpassante = _g->enpassante;
  g->castling = _g->castling;
  g->color = _g->color;
  g->halfclock = _g->halfclock;
  g->fullclock = _g->fullclock;
  g->npieces = _g->npieces;
  for(int i=0; i<2; ++i) {
    unsigned char k = _g->king[i].square;
    g->king[i] = &(board[k/8][k%8])
  }
 
  return g; 
}

#define TREE_DEPTH_MAX 3

typedef struct _TreeNode{
  _Game * g;
  unsigned int gstatus;

  //depth
  unsigned char depth; // [0,8) ?? 

  //Graph Connection using pointers
  unsigned char nchildren;
  struct _TreeNode * parent; 
  struct _TreeNode * sibling; 
  struct _TreeNode * children; 
} TreeNode;

typedef struct {
  _TreeNode * root;
} _Tree;

int TreeNodeCreate(_TreeNode * node) {
  _Game * g = node->g;

  //to avoid uncontrolled expansion of tree 
  assert(node->depth < TREE_MAX_DEPTH); 
  
  //avoiding multiple mem alloc 
  assert(!node->children); 

  //The game is over. No more moves.
  if(root->status) 
    return 0;

  int nmoves = (int) (g->moves->len / sizeof (_GameMove));
  _GameMove * move = (_GameMove *) (g->moves->p);

  _TreeNode * child = 
    (_TreeNode *) malloc(nmoves * sizeof (_TreeNode)); 

  for (int i=0; i<nmoves; ++i, ++child, ++move) {
    child->depth = node->depth + 1;
    child->children = NULL;
    child->parent = node;
    child->g = _GameCopy(g);
    child->g->move = move;
    child->status = GameMove(child->g); 
    //move "root" from one 
  }

  node->children = child;
  
  return nmoves;
}

_Tree * Tree(_Game * current) {
  _Tree * t = 
    (_Tree *) malloc (sizeof(_Tree));
  _Tree * root = 
    (_TreeNode *) malloc (sizeof(_TreeNode));
  _Game * g = GameCopy(current);

  root->g = g;
  root->parent = NULL;
  int nmoves = (int ) (g->moves->len) / sizeof (_GameMove);
  root->children = !nmoves ? NULL :
    ((_TreeNode **) malloc (nmoves sizeof(_TreeNode *)));

  t->root = root; 
}
