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
#define IS_LEAF_NODE   1
#define IS_ROOT_NODE   2
#define IS_PARENT_NODE 4
/* Nodes that are eligible to have children but not assigned*/
#define IS_PRUNED_NODE 32

_TreeNode * TREE_STACK[TREE_DEPTH_MAX];

typedef struct _TreeNode{
  _Game * g;
  unsigned int gstatus;

  //level. level in [0, depth)
  unsigned char level; // [0,8) ??
  unsigned char flags; //identify  

  //Graph Connection using pointers
  unsigned char nchildren;
  struct _TreeNode * parent; 
  struct _TreeNode * children; 
} TreeNode;

typedef struct {
  //Usually used to store current state of the game
  _TreeNode * root;    
  //max depth of search (further from current state)
  unsigned char depth; 
} _Tree;

int TreeNodeExpand(_TreeNode * node) {
  _Game * g = node->g;

  //to avoid uncontrolled expansion of tree 
  assert( node->depth < TREE_MAX_DEPTH );
 
  //avoiding multiple expansion of a leaf node; 
  assert( node->flags & IS_PARENT_NODE );
  assert( !node->children );

  //The game is over. Ex: No more moves (g->moves->len == 0).
  if(node->status) 
    return 0;

  //Looking for unexpected behaviour.
  assert(g->moves->len > 0);

  _GameMove * move = (_GameMove *) (g->moves->p);

  _TreeNode * child = 
    (_TreeNode *)malloc( nmoves * sizeof (_TreeNode) ); 
  node->children = child;

  for (int i=0; i<nmoves; ++i, ++child, ++move) {
    child->level    = node->level + 1;
    child->children = NULL; //Not yet assigned
    child->parent   = node;
    child->flags    = IS_LEAF_NODE;
    child->g        = _GameCopy(g);
    //"child" is one "move" (halfmove) ahead of the "node"
    child->status   = GameMove(child->g, move); 
  }

  //Not a child anymore;
  node->flags &= ~IS_LEAF_NODE;
  node->flags |=  IS_PARENT_NODE;
}

//Destroy Leaf Nodes;
void TreeNodeDestroy(_TreeNode * node) {
  //Make sure that it's leaf node; 
  assert(!node->children);
  GameDestroy(node->g);
  node->g = NULL;
  return;  
}

//Destroy children
void TreeNodePrune(_TreeNode * node) {
  assert( node->flags & IS_PARENT_NODE ) 
  _GameNode * child = node->children;
  for(int i=0; i<nchildren; ++i, ++child) {
    assert(child->flags & IS_LEAF_NODE);
    TreeNodeDestroy( child );
  }
  //No more a parent;
  node->flags &= ~IS_PARENT_NODE;
  node->flags |=  IS_LEAF_NODE;
  //free children
  free(node->children);
  node->children = NULL;
}

_Tree * Tree(_Game * g, unsigned char depth) {
  _Tree * tree     = (_Tree *) malloc (sizeof(_Tree));
  _TreeNode * root = (_TreeNode *) malloc (sizeof(_TreeNode));
  root->g = GameCopy(g);
  root->parent = NULL;
  root->children = NULL;
  root->level = 0;
  tree->root = root; 
  tree->depth = 
    depth > TREE_MAX_DEPTH ? TREE_MAX_DEPTH ? depth;
  for(int l=0; l<tree-depth; ++l) {
    foreach_TreeNode_start(tree, tree->depth) {
      if( node->flags & IS_PRUNED_NODE ) {
        assert(node->level == l);
        TreeNodeExpand(node); 
      }
    }
    foreach_TreeNode_end();
  }
  return tree;
}

