#include "move.h"

/**
TODO: mempool for board and moves. Cap a limit too the pool.
Parallel Tree Generation, Traversal etc
*/


/* ---------------------------------------------------------
------------------------------------------------------------
  A _TreeNode is used to keep an instance of the game.
  A node can expand as a tree to store different evolutions
  of the current board. 
  Node can be identified as leaf or internal node.
  There is exactly one root node.
  Nodes that are eligible to have children but not yet ..
  .. assigned with "children" are called PRUNED nodes. ..
  .. IS_PRUNED_NODE maybe synonymous with IS_LEAF_NODE, but
  .. is used to signify that algorithm prefered to not ..
  .. expand further from that node. 
------------------------------------------------------------
--------------------------------------------------------- */
typedef struct _Tree{
  //size_t id; 
  _Board * board;

  //Tree Connection using pointers
  struct _Tree * parent;   //parent node
  struct _Tree * children; //children

  Flag depth;     // depth + level <= TREE_MAX_DEPTH
  Flag level;     // level \in [0, TREE_MAX_DEPTH) 
  Flag flags;     // identify type of node
  Flag nchildren; // number of possible moves 
 
  int eval;   //board evaluation value \in [-5000, 5000]
} _TreeNode;

#define TREE_MAX_DEPTH 4
#define TREE_MIN_DEPTH 2
#define IS_LEAF_NODE   1
#define IS_ROOT_NODE   2
#define IS_PARENT_NODE 4
#define IS_NODE_ORIGINAL 16
#define IS_PRUNED_NODE 32

/* ---------------------------------------------------------
------------------------------------------------------------
  Assigne values to a _TreeNode object.
  (NOTE Doesn't create an object here).
------------------------------------------------------------
--------------------------------------------------------- */

void TreeNode(_Tree * node, _Tree * parent, 
    _Board * b, _BoardMove * move) {

  assert(b->status == GAME_CONTINUE);

  node->depth = 0;
  node->flags = IS_LEAF_NODE;
  node->nchildren = 0;
  // create new board & copy content from 'b'
  node->board = Board(b); 
  if(parent == NULL) { 
    node->flags |= IS_ROOT_NODE;
    node->level = 0; 
  }
  else {
    node->level = parent->level + 1;
    BoardMove(board, move);
    BoardUpdateMetadata(board, move);
    //NOTE: Status Not evaluated. Moves not created
  }
  node->parent = parent;
  node->children = NULL;

  return;
}


/* ---------------------------------------------------------
------------------------------------------------------------
  Free Memory associated with a Node 
  (NOTE: But not the node itself)
------------------------------------------------------------
--------------------------------------------------------- */

//Destroy Leaf Nodes;
Flag TreeNodeDestroy(_Tree * node) {
  // Make sure that it's leaf node; 
  assert( node->flags & IS_LEAF_NODE );
  assert( node->children == NULL );
  // Destroy associated memory created for "node". ..
  // Note: Assume memory for "node->children" are  ..
  // .. destroyed before.
  BoardDestroy( node->board );
  node->board = NULL;
  return 1;  
}

/* ---------------------------------------------------------
------------------------------------------------------------
  Create children for a _TreeNode by expanding using all ..
  .. possible moves.
------------------------------------------------------------
--------------------------------------------------------- */

Array MOVES_ARRAY = {.p = NULL, .len = 0, .max = 0};


Flag TreeNodeExpand(_Tree * node) {

  // Check for unexpected behaviour.
  assert( node->flags & IS_LEAF_NODE );
  assert( node->children == NULL );
  _Board * b = node->board;
  assert( b->status == GAME_STATUS_NOTUPDATED );
  // Once moves created, status is also updated.
  BoardAllMoves( b, &MOVES_ARRAY );
  
  if( b->status != GAME_CONTINUE ) {
    return 0; // cannot expand as the game is over
  } else if ( node->level == TREE_MAX_DEPTH ){
    node->flags |= IS_PRUNED_NODE;
    return 0; // cannot expand bcs of tree depth limit
  } else if ( node->level > TREE_MAX_DEPTH ) {
    fprintf(stderr, "ERROR: impossible tree depth!!");
    fflush(stderr);
    exit(-1);
  }

  assert(MOVES_ARRAY.len);

  _GameMove * move = (_GameMove *) (MOVES_ARRAY.p);
  Flag nmoves = (Flag) (MOVES_ARRAY.len/sizeof(_GameMove));
  _Tree * child  = (_Tree *) malloc (nmoves * sizeof (_Tree)); 

  node->nchildren = nmoves; 
  node->children = child;

  for (int i=0; i<nmoves; ++i, ++child, ++move) 
    TreeNode(child, node, node->board, move);

  //node is not a leaf anymore;
  node->flags &= ~(IS_LEAF_NODE | IS_PRUNED_NODE);
  node->flags |=  IS_PARENT_NODE;
  node->depth =  1;
  
  return nmoves;
}
/* ---------------------------------------------------------
------------------------------------------------------------
  Pruning (or undoing a node expansion). 
  It can be done only if all children are leaves;
------------------------------------------------------------
--------------------------------------------------------- */

//free "children"
int TreeNodePrune(_TreeNode * node) {
  if( node->depth != 1 ) { 
    /* Inorder to prune a node, each it's children ..
    .. should be a LEAF (depth = 0) */
    return 0; 
  }
 
  _TreeNode * child = node->children;
  assert(child);
  //free "children" and associated memory for it's objects.
  for(int i=0; i<node->nchildren; ++i, ++child) {
    assert( child->flags & IS_LEAF_NODE ); 
    TreeNodeDestroy( child );
  }
  free(node->children);
  node->children = NULL;
  // "node" is no  more a PARENT,
  // "node" is a PRUNED LEAF (depth = 0)
  node->flags &= ~IS_PARENT_NODE;
  node->flags |=  (IS_LEAF_NODE | IS_PRUNED_NODE);
  node->depth = 0;
  
  return 1;
}


/* ---------------------------------------------------------
------------------------------------------------------------
Tree Traversal without a recursion function.
1) Depth-First Search (Pre-Order) is implemented in ..
   .. TreeEachNode(). Is used to create the tree, etc.
     1
    / \
   2   5
  / \
 3  4
2) Depth-First Search (Post-Order). 
   Refer TreeEachNodePostOrder(). 
   Is used for reduction operation vertically up.
    Ex: f(parent) = max {f(children)}
   as in minimax algorithm.
   Is also used prune each subtree, in one go.
     5
    / \
   3   4
  / \
 1  2
------------------------------------------------------------
--------------------------------------------------------- */

Flag TREE_STACK[TREE_MAX_DEPTH];
typedef int (* TreeNodeFunction) (_TreeNode * node); 

void TreeEachNode(_Tree * tree, unsigned char depth, 
    TreeNodeFunction func){

  assert(tree);                                 
  assert(depth <= tree->depth);               
  TREE_STACK[0] = 1; 
  while(node) {                                 
    while(node->level <= depth) {
      /* Do something with node here  */
      if(func)
        func(node);
      /* End of "Do something with node here"*/  
      
      /* Going down the tree */
      if(node->depth == 0) {  //Cannot go down further
        assert(node->children == NULL);
        assert(node->flags & IS_LEAF_NODE)
        break; 
      }
      else if(node->level <= depth) {
        //Go to children if node->level is not yet "depth"
        TREE_STACK[node->level + 1] = node->nchildren;
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
      _Tree * parent = node->parent;
      if(parent) {
        // Making sure depth is always updated  as the tree ..
        // .. is dynamic.
        Flag parentDepth = node->depth + 1;
        parent->depth =
  (node == parent->children) ? parentDepth :    //1st child? 
  (parent->depth < parentDepth) ? parentDepth : //update
  parent->depth;                                //no change
      }
      node = parent;
    } 
  }
}

//typedef int (* TreeNodeReductionFunction) (_TreeNode *, void *); 

void TreeEachNodePostOrder(_Tree * tree, unsigned char depth, 
    TreeNodeFunction rfunc){

  assert(tree);                                 
  assert(depth <= tree->depthmax);               
  _TreeNode * node = tree->root;             
  TREE_STACK[0] = 1; 
  while(node) {                                 
    while(node->level <= depth) {
      
      /* Going down the tree */
      if(!node->children) {
        break; //Cannot go down further
      }
      else if(node->level <= depth) {
        //Go to children if node->level is not yet "depth"
        TREE_STACK[node->level + 1] = node->nchildren;
        node = node->children;
      } 
    }

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( node ) {
      /* Do something with node here  */
      if(rfunc)
        rfunc(node);
      /* End of "Do something with node here"*/ 
 
      if( --TREE_STACK[node->level] > 0 ) {  
        ++node; 
        break;
      }
      _Tree * parent = node->parent;
      if(parent) {
        // Making sure depth is always updated  as the tree ..
        // .. is dynamic.
        Flag parentDepth = node->depth + 1;
        parent->depth =
  (node == parent->children) ? parentDepth :    //1st child? 
  (parent->depth < parentDepth) ? parentDepth : //update
  parent->depth;                                //no change
      }
      node = parent;
    } 
  }
}

/* ---------------------------------------------------------
------------------------------------------------------------
  Create a tree from a given instance of game 
------------------------------------------------------------
--------------------------------------------------------- */

_Tree * Tree(_Board * b, Flag depth) {
  //srand(time(0));  
  if(b->status) {
    //Game already over. No scope to expand a tree
    return NULL;
  }

  _Tree * root = (_Tree *) malloc (sizeof(_Tree));
  TreeNode(root, NULL, b, NULL);
  depth = (depth > TREE_MAX_DEPTH) ? TREE_MAX_DEPTH : 
          (depth < TREE_MIN_DEPTH) ? TREE_MIN_DEPTH : depth;

  TreeEachNode(tree, depth - 1, TreeNodeExpand);
  // There is no pruning, .. thus around 20^TREE_MAX_DEPTH 
  // is the number of leaf nodes. Make sure you don't ..
  // .. run out RAM and crash the system.
 
  return root;
}

void TreeDestroy(_Tree * tree) { 

  //  You can prune the entire tree in one go using ..
  //  .. DFS - (post order) traversal
  TreeEachNodePostOrder(tree, tree->depth, 
    TreeNodePrune);
  
  //  Other wise you have to do it in different levels ..
  //  starting from bottom
  //  (Not the ideal one; 
  //  for (int l=tree->depth-1; l>0; --l)
  //    TreeEachNode(tree, l, TreeNodePrune);

  TreeNodeDestroy(tree);
  free(tree);
}
