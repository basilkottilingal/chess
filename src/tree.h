#include "move.h"

/**
TODO: mempool for board and moves. Cap a limit too the pool.
*/

_Game * GameCopy(_Game * _g) {
  //Game instance 'g' copied from '_g'
  _Game * g = (_Game *) malloc (sizeof (_Game));

  g->board = GameBoard();

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
/* Nodes that are eligible to have children but not yet ..
.. assigned with "children" are called PRUNED nodes. ..
.. IS_PRUNED_NODE is synonymous with IS_LEAF_NODE, but
.. is used to signify that algorithm prefered to not ..
.. expand further from that node. */
#define IS_PRUNED_NODE 32


/* Depth-First Tree Traversal without a recursion function */
unsigned char TREE_STACK[TREE_DEPTH_MAX];
void TreeNodeEach(_Tree * tree, int depth, func){
  assert(tree);                                 
  //assert(depth <= tree->depth);               
  _TreeNode * node_ = tree->root;             
  TREE_STACK[0] = 1; 
  while(node) {                                 
    while(node->level <= depth) {
      /* Do something with node here */  
      //func(node, data);
      fprintf(stdout, "\n");
      for(int i=0; i<node->level; ++i)
        fprintf(stdout, " ");
      fprintf(stdout, "l%d,flags%d", node->level, node->flags);
      /* End of "Do something with node here"*/  
      
      /* Going down the tree */
      if(!node->children) {
        break; //Cannot go down further
      }
      else if(node->level < depth) {
        //Go to children if node->level is not yet "depth"
        TREE_STACK[TREE_LEVEL + 1] = node->nchildren;
        node = node->children;
      } 
    }

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( node ) {
      if( --TREE_STACK[node->level] > 0 ) {  
        ++node; 
        break;
      }
      node = node->parent;
    } 
  }
}

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
  assert( node->flags & (IS_LEAF_NODE | IS_PRUNED_NODE) );
  assert( !node->children );

  //The game is over. Ex: No more moves (g->moves->len == 0).
  if(node->status) 
    return 0;

  //Looking for unexpected behaviour.
  assert(g->moves->len > 0);

  _GameMove * move = (_GameMove *) (g->moves->p);
  _TreeNode * child = 
    (_TreeNode *)malloc(node->nchildren * sizeof (_TreeNode)); 
  node->children = child;

  for (int i=0; i<nmoves; ++i, ++child, ++move) {
    child->level     = node->level + 1;
    child->children  = NULL; //Not yet assigned
    child->parent    = node;
    child->flags     = IS_LEAF_NODE;
    child->nchildren = 0
    //"child" is one "move" (halfmove) ahead of the "node"
    child->g         = _GameCopy(g);
    child->status    = GameMove(child->g, move);
    if(!child->status) {
      //this child can be further expanded
      child->flags  |= IS_PRUNED_NODE;
      child->nchildren = 
        (unsigned char) (child->g->moves / sizeof(_GameMove));
    }
  }

  //Not a child anymore;
  node->flags &= ~(IS_LEAF_NODE & IS_PRUNED_NODE);
  node->flags |=  IS_PARENT_NODE;
}

//Destroy Leaf Nodes;
void TreeNodeDestroy(_TreeNode * node) {
  //Make sure that it's leaf node; 
  assert(child->flags & IS_LEAF_NODE);
  assert(!node->children);
  // Destroyed associated memory created for "node". ..
  // Note: Assume memory for "node->children" are  ..
  // .. destroyed before.
  GameDestroy(node->g);
  node->g = NULL;
  return;  
}

//free "children"
void TreeNodePrune(_TreeNode * node) {
  assert( node->flags & IS_PARENT_NODE ) 
  _GameNode * child = node->children;
  assert(child);
  //free "children" and associated memory for it's objects.
  for(int i=0; i<nchildren; ++i, ++child) 
    TreeNodeDestroy( child );
  free(node->children);
  node->children = NULL;
  //No more a parent;
  node->flags &= ~IS_PARENT_NODE;
  node->flags |=  (IS_LEAF_NODE | IS_PRUNED_NODE);
}

_Tree * Tree(_Game * g, unsigned char depth) {
  if(g->gstatus) {
    //Game already over. No scope to expand a tree
    return NULL;
  }

  _Tree * tree     = (_Tree *) malloc (sizeof(_Tree));
  _TreeNode * root = (_TreeNode *) malloc (sizeof(_TreeNode));
  root->g = GameCopy(g);
  root->parent = NULL;
  root->children = NULL;
  root->level = 0;
  tree->root = root; 
  tree->depth = 
    depth > TREE_MAX_DEPTH ? TREE_MAX_DEPTH ? depth;

  //FIXME: Slow algorithm.
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

