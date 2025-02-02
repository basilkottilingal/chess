#include "game.h"
#include "mempool.h"

/**
TODO: mempool for board and moves. Cap a limit too the pool.
Parallel Tree Generation, Traversal etc
*/


/* ---------------------------------------------------------
------------------------------------------------------------
  A _Tree is used to keep an instance of the game.
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
typedef struct _Tree {
  //size_t id; 
  _Board board;

  //Tree Connection using pointers
  struct _Tree * parent;   //parent node
  struct _Tree ** children; //children
                          
  Flag depth, depthmax;   // depth <= depthmax
  // depthmax is the max allowed depth. 
  // For root node, depthmax <= TREE_MAX_DEPTH
  Flag level;             // level \in [0, root->depth) 
  Flag flags;             // identify type of node
  Flag nchildren;         // number of possible moves
  Flag ichild;            // which child is chosen by engine.
 
  int eval;   //board evaluation value \in [-5000, 5000]
} _Tree;

_Mempool * TreePool = NULL;

static inline size_t TreeAvailability(){
  return TreePool->nfree;
}

_Tree * TreeFromPool() {
  if(!TreePool) {
    TreePool = 
      Mempool(sizeof(_Tree), 1 + 1024*1024/sizeof(_Tree)); 
      //1MB  guaranteed
    fprintf(stdout, "\nTreePool avilability %ld", 
      TreeAvailability()); 
    fflush(stdout);
  }
  return (_Tree *) MempoolAllocateFrom (TreePool);  
}

void TreeToPool(_Tree * tree) {
  MempoolDeallocateTo (TreePool, tree);  
}

#ifndef TREE_MAX_DEPTH
#define TREE_MAX_DEPTH 4 
#endif
#ifndef TREE_MIN_DEPTH
#define TREE_MIN_DEPTH 2 
#endif

enum TREE_NODE_FLAG {
  IS_LEAF_NODE = 1,
  IS_ROOT_NODE = 2,
  IS_PARENT_NODE = 4,
  //IS_NODE_ORIGINAL = 16,
  IS_PRUNED_NODE = 32
};
Array MOVES_ARRAY = {.p = NULL, .len = 0, .max = 0};

/* ---------------------------------------------------------
------------------------------------------------------------
  Assigne values to a _Tree object.
  (NOTE Doesn't create an object here).
------------------------------------------------------------
--------------------------------------------------------- */

void TreeRootNode(_Tree * root, _Board * b, Flag depthmax) {
  assert(b->status == GAME_CONTINUE);

  root->depth = 0;
  root->depthmax = depthmax;
  root->level = 0; 
  root->flags = IS_LEAF_NODE | IS_ROOT_NODE;
  root->nchildren = 0;
  // Tree connection
  root->parent = NULL;
  root->children = NULL;
  // create new board & copy content from 'b'
  //root->board = Board(b); 
  BoardCopy(&root->board, b);
}

void TreeChildNode(_Tree * child, _Tree * parent, 
    _BoardMove * move) {

  assert(parent && move);
  assert(parent->board.status == GAME_CONTINUE);

  child->depth = 0;
  child->depthmax = parent->depthmax - 1;
  child->level = parent->level + 1;
  child->flags = IS_LEAF_NODE;
  child->nchildren = 0;
  //Tree connectivity
  child->parent = parent;
  child->children = NULL;
  //Create the board of the child.
  _Board * board = &child->board;
  BoardCopy(board, &parent->board);
  BoardMove(board, move);
  BoardUpdateMetadata(board, move); 
  //NOTE:Status not updated yet. Moves not created
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
  //BoardDestroy( node->board );
  //node->board = NULL;
  TreeToPool(node);
  return 1;  
}

/* ---------------------------------------------------------
------------------------------------------------------------
  Create children for a _Tree by expanding using all ..
  .. possible moves.
------------------------------------------------------------
--------------------------------------------------------- */



Flag TreeNodeExpand(_Tree * node) {

  // Check for unexpected behaviour.
  assert( node->flags & IS_LEAF_NODE );
  assert( node->children == NULL );
  //assert( node->board->status == GAME_STATUS_NOTUPDATED );

  _Board * b = &(node->board);
  // Once moves created, status is also updated.
  BoardAllMoves( b, &MOVES_ARRAY );
  
  if( b->status != GAME_CONTINUE ) {
    return 0; // cannot expand as the game is over
  } else if ( node->depthmax == 0 ){
    node->flags |= IS_PRUNED_NODE;
    return 0; // cannot expand bcs of tree depth limit
  }

  assert(MOVES_ARRAY.len);
  _BoardMove * move = (_BoardMove *) (MOVES_ARRAY.p);
  Flag nmoves = (Flag) (MOVES_ARRAY.len/sizeof(_BoardMove));
  if( TreeAvailability() < (size_t) nmoves) {
    fprintf(stderr, "\nWARNING: Ran out of Tree Memory Block");
    node->flags |= IS_PRUNED_NODE;
    return 0; // cannot expand bcs ran out of memory
  }

  _Tree ** children = 
    (_Tree **) malloc ((1 + nmoves) * sizeof (_Tree *)); 

  node->nchildren  = nmoves; 
  node->children   = children;
  children[nmoves] = NULL; //end of child

  // Set child properties
  for (int i=0; i<nmoves; ++i, ++children, ++move) {
    *children = TreeFromPool();
    assert(*children);
    TreeChildNode(*children, node, move);
  }
 
  // In case children are at max depth
  children = node->children;
  for (int i=0; i<nmoves; ++i, ++children)  {
    _Tree * child = *children;
    if(child->depthmax == 0)  
      // If not allowed to expand further, update the ..
      // .. board->status by calling BoardAllMoves();
      //if(!BoardAllMoves(child->board, &MOVES_ARRAY))
      if(!BoardAllMoves(&child->board, &MOVES_ARRAY))
        //i.e (status == GAME_CONTINUE)
        child->flags |= IS_PRUNED_NODE;
      // else 
      //  No possibility to expand any way: The game is OVER
  }

  //node is not a leaf anymore;
  node->flags &= ~(IS_LEAF_NODE | IS_PRUNED_NODE);
  node->flags |=  IS_PARENT_NODE;
  node->depth = 1;
  
  return nmoves;
}
/* ---------------------------------------------------------
------------------------------------------------------------
  Pruning (or undoing a node expansion). 
  It can be done only if all children are leaves;
------------------------------------------------------------
--------------------------------------------------------- */

//free "children"
Flag TreeNodePrune(_Tree * node) {
  if( node->depth != 1 ) { 
    /* Inorder to prune a node, each it's children ..
    .. should be a LEAF (depth = 0) */
    return 0; 
  }
 
  _Tree ** children = node->children;
  assert(children);
  //free "children" and associated memory for it's objects.
  for(int i=0; i<node->nchildren; ++i, ++children) {
    _Tree * child = *children;
    assert( child->flags & IS_LEAF_NODE ); 
    assert( child->depth == 0);
    TreeNodeDestroy( child );
    *children = NULL;
  }
  free(node->children);
  node->children = NULL;
  node->nchildren = 0;
  // "node" is no  more a PARENT,
  // "node" is a PRUNED LEAF (depth = 0)
  node->flags &= ~IS_PARENT_NODE;
  node->flags |=  (IS_LEAF_NODE | IS_PRUNED_NODE);
  node->depth = 0;
  
  return 1;
}

static inline 
void TreeUpdateParentDepth(_Tree * child) {
  _Tree * parent = child->parent;
  if(!parent) 
    return;
    
  // Making sure depth is always updated  as the tree ..
  // .. is dynamic.
  Flag parentDepth = child->depth + 1;
  parent->depth =
    (child == parent->children[0]) ? parentDepth :    
    (parent->depth < parentDepth) ? parentDepth : 
     parent->depth;
}


/* ---------------------------------------------------------
------------------------------------------------------------
Tree Traversal without a recursion function.
1) Depth-First Search (Pre-Order) is implemented in ..
   .. TreeEachNode(). Is used (a) to create the tree, 
   .. (b) Inherit something from parent .
   .. Ex: f(child) = Inherit(f(parent));
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

typedef Flag (* TreeNodeFunction) (_Tree * node); 

void TreeEachNode(_Tree * root, 
    Flag depth, TreeNodeFunction func){

  assert(root);                                 
  //depth = depth + node->level; //in case node is not the root
  assert(depth <= root->depthmax);               
  _Tree * node = root, ** stack[TREE_MAX_DEPTH + 1],
    ** rootParent = (_Tree **) malloc(2*sizeof(_Tree *));
  rootParent[0] = root;   rootParent[1] = NULL;
  int level = 0;

  //i don;t like it
  stack[level]  = rootParent; 
 
  // fixme: make it general for any subtree (even node!=root)
  // while(TREE_STACK[node->level] >= 0) 
  while(level >= 0) { 

    while(level <= depth) {
      /* Do something with node here  */
      if(func)
        func(node);
      /* End of "Do something with node here"*/  
      
      /* Going down the tree */
      if(node->depth == 0) {  //Cannot go down further
        assert(node->children == NULL);
        assert(node->flags & IS_LEAF_NODE);
        break; 
      }
      else if(level <= depth) {
        //Go to 1st child if node->level is not yet "depth"
        stack[++level] = node->children;
        node = node->children[0];
      } 
    }

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( level >= 0 ) {
      TreeUpdateParentDepth(node);
      ++(stack[level]); 
      if( *(stack[level]) ) {  
        node = *(stack[level]); 
        break;
      }
      node = node->parent;
      --level;
    } 
  }

  free(rootParent);
}


void TreeEachNodePostOrder(_Tree * root, Flag depth, 
    TreeNodeFunction rfunc){


  assert(root);                                 
  //depth = depth + node->level; //in case node is not the root
  assert(depth <= root->depthmax);               
  _Tree * node = root, ** stack[TREE_MAX_DEPTH + 1],
    ** rootParent = (_Tree **) malloc(2*sizeof(_Tree *));
  rootParent[0] = root;   rootParent[1] = NULL;
  int level = 0;

  //i don;t like it
  stack[level]  = rootParent; 
 
  // fixme: make it general for any subtree (even node!=root)
  // while(TREE_STACK[node->level] >= 0) 
  while(level >= 0) { 

    while(level <= depth) {
      
      /* Going down the tree */
      if(node->depth == 0) {  //Cannot go down further
        assert(node->children == NULL);
        assert(node->flags & IS_LEAF_NODE);
        break; 
      }
      else if(level <= depth) {
        //Go to 1st child if node->level is not yet "depth"
        stack[++level] = node->children;
        node = node->children[0];
      } 
    }

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( level >= 0 ) {
      /* Do something with node here  */
      if(rfunc)
        rfunc(node);
      /* End of "Do something with node here"*/  

      TreeUpdateParentDepth(node);
      ++(stack[level]); 
      if( *(stack[level]) ) {  
        node = *(stack[level]); 
        break;
      }
      node = node->parent;
      --level;
    } 
  }

  free(rootParent);
}

/* ---------------------------------------------------------
------------------------------------------------------------
  Create a tree from a given instance of game 
------------------------------------------------------------
--------------------------------------------------------- */

_Tree * Tree(_Board * b, Flag depthmax) {
  if( b->status & (GAME_IS_A_WIN | GAME_IS_A_DRAW) ) {
    fprintf(stderr, "WARNING: Game Over, \
Can't create a game tree");
    fflush(stderr);
    return NULL;
  }
  else if(b->status & 
      (GAME_METADATA_NOTUPDATED | GAME_STATUS_NOTUPDATED)) {
    fprintf(stderr, "WARNING: Incomplete move, \
Can't create a game tree");
    fflush(stderr);
    return NULL;
  }

#if 1
  _Tree * root = TreeFromPool();
  if(!root) 
    return NULL;
#else
  _Tree * root = (_Tree *) malloc (sizeof(_Tree));
#endif
  depthmax = 
    (depthmax > TREE_MAX_DEPTH) ? TREE_MAX_DEPTH : 
    (depthmax < TREE_MIN_DEPTH) ? TREE_MIN_DEPTH : depthmax;

  //Assign values for the root node.
  TreeRootNode(root, b, depthmax);

  //Create the full tree upto depthmax
  TreeEachNode(root, depthmax - 1, TreeNodeExpand);
  // If no pruning, expect around (20^TREE_MAX_DEPTH)
  // number of leaf nodes. Make sure you don't ..
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
}


Flag TreeNodeCheckFlags(_Tree * node) {
  /* For debugging */
  _Board * b = &(node->board);
  assert(b);

  /* Verifying inequalities related to level, depth, ..*/
  assert(node->level + node->depth <= TREE_MAX_DEPTH);
  assert(node->depthmax <= TREE_MAX_DEPTH); 

  /* Verifying flags and game status */
  Flag flags = node->flags, status = b->status;
  if(flags & IS_LEAF_NODE) {
    assert(!node->children);
    assert(!node->nchildren);
    assert(!node->depth);
  }
  if(flags & IS_ROOT_NODE) {
    assert(!node->level);
    assert(!node->parent);
  }
  if(flags & IS_PARENT_NODE) { 
    assert(node->children);
    assert(status == GAME_CONTINUE);
    assert(node->depth); // >= 0
  }
  if(flags & IS_PRUNED_NODE) {
    //Pruned means "undeveloped" thus a leaf node
    assert(flags & IS_LEAF_NODE);
    assert(status == GAME_CONTINUE);
  }
  else if(flags & IS_LEAF_NODE)  //Not a pruned leaf
    //Exhaustive n exclusive win/draw
    assert((status & GAME_IS_A_WIN)^(status & GAME_IS_A_DRAW));
  //Exhaustive+exclusive nature of leaf/parent node
  assert( (flags&IS_LEAF_NODE) ^ (flags&IS_PARENT_NODE) );
  
  return 1;
}

void TreeDebug(_Tree * root) {
  TreeEachNode(root, root->depth, TreeNodeCheckFlags);
}
