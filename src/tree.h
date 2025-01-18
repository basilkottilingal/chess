#include "move.h"

_Game * GameCopy(_Game * _g) {
  //Game instance 'g' copied from '_g'
  _Game * g = (_Game *) malloc (sizeof (_Game));

  g->board = GameBoard();;

  //board is copied
  memcpy( &(board[-2][-2]), &(_g->board[-2][-2]), 
    144*sizeof(_GameSquare) );

  //Not used.
  g->history = NULL; 

  // Copy moves from "_g->moves" rather than generating moves
  g->moves = array_new();
  array_append(g->moves, _g->moves->p, _g->moves->len);
  // shrink array to avoid using larger memory for ..
  // .. smaller sized data
  array_shrink(g->moves);

  //Game meta data 
  g->check      = _g->check;
  g->enpassante = _g->enpassante;
  g->castling   = _g->castling;
  g->color      = _g->color;
  g->halfclock  = _g->halfclock;
  g->fullclock  = _g->fullclock;
  g->npieces    = _g->npieces;
  for(int i=0; i<2; ++i) {
    unsigned char k = _g->king[i].square;
    g->king[i] = &(board[k/8][k%8])
  }
 
  return g; 
}

#define TREE_DEPTH_MAX 4

_TreeNode * TREE_STACK[TREE_DEPTH_MAX];

typedef struct _TreeNode{
  _Game * g;
  unsigned int gstatus;

  //level. level in [0, depth)
  unsigned char level; // [0,8) ?? 

  //Graph Connection using pointers
  unsigned char nchildren;
  struct _TreeNode * parent; 
  struct _TreeNode * sibling; 
  struct _TreeNode * children; 
} TreeNode;

typedef struct {
  //Usually used to store current state of the game
  _TreeNode * root;    
  //max depth of search (further from current state)
  unsigned char depth; 
} _Tree;

int TreeNodeCreate(_TreeNode * node) {
  _Game * g = node->g;

  //to avoid uncontrolled expansion of tree 
  assert(node->depth < TREE_MAX_DEPTH); 
  
  //avoiding multiple mem alloc 
  assert(!node->children); 

  //The game is over. Ex: No more moves (g->moves->len == 0).
  if(root->status) 
    return 0;

  //Looking for unexpeccted behaviour.
  assert(g->moves->len > 0);

  int nmoves = (int) (g->moves->len / sizeof (_GameMove));
  _GameMove * move = (_GameMove *) (g->moves->p);

  _TreeNode * child = 
    (_TreeNode *) malloc(nmoves * sizeof (_TreeNode)); 

  for (int i=0; i<nmoves; ++i, ++child, ++move) {
    child->level = node->level + 1;
    child->children = NULL; //Not yet assigned
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
