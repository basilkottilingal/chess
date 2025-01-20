#include "move.h"

/**
TODO: mempool for board and moves. Cap a limit too the pool.
*/

_Game * GameCopy(_Game * _g) {
  //Game instance 'g' copied from '_g'
  _Game * g = (_Game *) malloc (sizeof (_Game));

  g->board = GameBoard();

  //board is copied
  memcpy( &(g->board[-2][-2]), &(_g->board[-2][-2]), 
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
  g->status     = _g->status;
  g->enpassante = _g->enpassante;
  g->castling   = _g->castling;
  g->color      = _g->color;
  g->halfclock  = _g->halfclock;
  g->fullclock  = _g->fullclock;
  g->npieces    = _g->npieces;
  for(int i=0; i<2; ++i) {
    unsigned char k = _g->king[i]->square;
    g->king[i] = &(g->board[k/8][k%8]);
  }
 
  return g; 
}

#define TREE_MAX_DEPTH 4
#define IS_LEAF_NODE   1
#define IS_ROOT_NODE   2
#define IS_PARENT_NODE 4
/* Nodes that are eligible to have children but not yet ..
.. assigned with "children" are called PRUNED nodes. ..
.. IS_PRUNED_NODE is synonymous with IS_LEAF_NODE, but
.. is used to signify that algorithm prefered to not ..
.. expand further from that node. */
#define IS_PRUNED_NODE 32


typedef struct _TreeNode{
unsigned int id; 
  _Game * g;
  //unsigned int gstatus;

  //level. level in [0, depth)
  unsigned char level; // [0,8) ??
  unsigned char flags; //identify  

  //Graph Connection using pointers
  unsigned char nchildren;
  struct _TreeNode * parent; 
  struct _TreeNode * children; 
} _TreeNode;

void TreeNode(_TreeNode * node, _TreeNode * parent, 
    _Game * g, _GameMove * move) {
node->id = rand();

  assert(!g->status);
  node->g = GameCopy(g);
  node->flags = IS_LEAF_NODE;
  if(parent == NULL) { 
    //New node is the root node
    node->flags |= IS_ROOT_NODE;
    node->level = 0;
  }
  else {
    node->level = parent->level + 1;
    GameMove(node->g, move);
  }
//fprintf(stdout, "NewNode @ %d", node->level);

  if(!node->g->status) {
    // This node is potential to expand; 
    // Or the game still not over.
    node->flags |= IS_PRUNED_NODE;
    node->nchildren = (unsigned char) 
      (node->g->moves->len /sizeof (_GameMove));
  }
  else {
    // Redundant; bcs Not used in case (node->children == NULL)
    node->nchildren = 0;
    // assume the node is not the root node.
    // Bcs, make a tree with no possible moves is useless.
    assert( !(node->flags & IS_ROOT_NODE) );  
  }

  //tree links
  node->parent = parent;
  node->children = NULL;

  return;
}

//Destroy Leaf Nodes;
int TreeNodeDestroy(_TreeNode * node) {
  // Make sure that it's leaf node; 
  assert( node->flags & IS_LEAF_NODE );
  assert( node->children == NULL );
  // Destroy associated memory created for "node". ..
  // Note: Assume memory for "node->children" are  ..
  // .. destroyed before.
  GameDestroy( node->g );
  node->g = NULL;
  return 1;  
}

int TreeNodeExpand(_TreeNode * node) {


  if( !(node->flags & IS_PRUNED_NODE) ) {
    return 0; //Cannot expand this node; 
  }

  //to avoid uncontrolled expansion of tree 
  assert( node->level < TREE_MAX_DEPTH );
  //avoiding multiple expansion of the same node; 
  assert( node->flags & (IS_LEAF_NODE | IS_PRUNED_NODE) );
  assert( node->children == NULL );
  //Looking for unexpected behaviour.
  assert( !node->g->status );
  assert( node->g->moves->len );

  _GameMove * move = (_GameMove *) (node->g->moves->p);
  unsigned char nmoves = node->nchildren;
//fprintf(stdout, "(Node-0x%d Expand %d-Children)", node->id,nmoves);
  _TreeNode * child = 
    (_TreeNode *) malloc (nmoves * sizeof (_TreeNode)); 
  node->children = child;

//fprintf(stdout, "{");
  for (int i=0; i<nmoves; ++i, ++child, ++move) {
    TreeNode(child, node, node->g, move);
//fprintf(stdout, "0x%d,", child->id);
  }
//fprintf(stdout, "}");

  //node is not a leaf anymore;
  node->flags &= ~(IS_LEAF_NODE & IS_PRUNED_NODE);
  node->flags |=  IS_PARENT_NODE;
  
  return nmoves;
}

//free "children"
int TreeNodePrune(_TreeNode * node) {
  if( !(node->flags & IS_PARENT_NODE )) 
    return 0;
 
  _TreeNode * child = node->children;
  assert(child);
  //free "children" and associated memory for it's objects.
  for(int i=0; i<node->nchildren; ++i, ++child) { 
    if(! (child->flags & IS_LEAF_NODE) ) {
      return 0;
    }
    TreeNodeDestroy( child );
  }
  free(node->children);
  node->children = NULL;
  //No more a parent;
  node->flags &= ~IS_PARENT_NODE;
  node->flags |=  (IS_LEAF_NODE | IS_PRUNED_NODE);
  
  return 1;
}

typedef struct {
  //Usually used to store current state of the game
  _TreeNode * root;    
  //max depth of search (further from current state)
  unsigned char depth, depthmax;
} _Tree;

typedef int (* TreeNodeFunction) (_TreeNode *); 
/* Depth-First Tree Traversal without a recursion function */
unsigned char TREE_STACK[TREE_MAX_DEPTH];
void TreeNodeEach(_Tree * tree, unsigned char depth, 
    TreeNodeFunction func){

  assert(tree);                                 
  //assert(depth <= tree->depth);               
  _TreeNode * node = tree->root;             
  TREE_STACK[0] = 1; 
  while(node) {                                 
    while(node->level <= depth) {
      /* Do something with node here  
      fprintf(stdout, "\n");
      for(int i=0; i<node->level; ++i)
        fprintf(stdout, " ");
      fprintf(stdout, "l%d,flags%d", node->level, node->flags);
      */
      if(func)
        func(node);
      /* //End of "Do something with node here"*/  
      
      /* Going down the tree */
      if(!node->children) {
        //fprintf(stdout,"<E>"); fflush(stdout);
        break; //Cannot go down further
      }
      else if(node->level <= depth) {
        //fprintf(stdout,"<D>"); fflush(stdout);
        //Go to children if node->level is not yet "depth"
        TREE_STACK[node->level + 1] = node->nchildren;
        node = node->children;
      } 
    }
        //fprintf(stdout,"<M>"); fflush(stdout);

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( node ) {
      if( --TREE_STACK[node->level] > 0 ) {  
        ++node; 
        //fprintf(stdout,"<X>"); fflush(stdout);
        break;
      }
      node = node->parent;
        //fprintf(stdout,"<Y>"); fflush(stdout);
    } 
        //fprintf(stdout,"<Z>"); fflush(stdout);
  }
        //fprintf(stdout,"<Q>"); fflush(stdout);
}

_Tree * Tree(_Game * g, unsigned char depth) {
srand(time(0));  
  if(g->status) {
    //Game already over. No scope to expand a tree
    return NULL;
  }

  _Tree * tree     = (_Tree *) malloc (sizeof(_Tree));
  _TreeNode * root = (_TreeNode *) malloc (sizeof(_TreeNode));
  TreeNode(root, NULL, g, NULL);
  tree->root = root;
  tree->depthmax = 
    depth > TREE_MAX_DEPTH ? TREE_MAX_DEPTH : depth;

  // Expand the tree
  TreeNodeEach(tree, TREE_MAX_DEPTH-1, TreeNodeExpand);
  // There is no pruning, .. thus around 20^TREE_MAX_DEPTH 
  // is the number of leaf nodes. Make sure you don't ..
  // .. run out RAM and crash the system.
  tree->depth = tree->depthmax;
 
  return tree;
}

void TreeDestroy(_Tree * tree) { 

  //Not the ideal one; 
  //You should be able to do it in one go like creatiing tree
  for (int l=tree->depth-1; l>0; --l)
    TreeNodeEach(tree, l, TreeNodePrune);

}
