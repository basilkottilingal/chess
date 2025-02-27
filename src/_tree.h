#include "move.h"
typedef struct _Node _Node;
typedef struct _Edge _Edge;

struct _Node{
  /* First child */
  _Edge * child;
  /* Data for the node */
  _Board board;
  /* Flags of the node */
  Flag flags;
  /* Depth of this node */
  Flag depth;
  /* Depth allowed */
  Flag depthmax;
  /* Number of children. */
  Flag nchildren;
};

struct _Edge {
  /* End node of this edge */
  _Node * node;
  /* Next Edge of this edge's parent */
  _Edge * sibling;
  /* Data for the edge */
  _Move move; 
};

typedef _Node _Tree;

_Tree * TreeIterator = NULL;

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

typedef Flag (* TreeNodeFunction) (_Node * node); 

_Edge * EdgeStack[TREE_MAX_DEPTH];

static inline   
_Edge * EdgeGotoParent(_Edge *edge, int * level) {
  /* It applies for root node only. ..
  .. which doesn't have a parent*/
  if(!*level--) 
    return NULL;

  /* Update the depth of parent */
  Flag depth = 0;
  _Edge * parent = EdgeStack[*level],
        * child = parent->node->child;
  while(child) {
    if(child->node)
      if(depth < child->node->depth)
        depth = child->node->depth;
    child = child->sibling;
  }
  parent->node->depth = 1 + depth;
  return parent;
}

static inline
_Edge * EdgeGotoChild(_Edge * edge, int * level) {
  assert(edge->node);
  _Edge * child = edge->node->child;
  if(!child)
    return NULL;
  //if(child->node) 
  //  child->node->depthmax = edge->node->depthmax - 1;
  EdgeStack[++*level] = child;
  return child;
}

static inline
_Edge * EdgeGotoSibling(_Edge * edge, int * level) {
  _Edge * sibling = edge->sibling;
  EdgeStack[*level] = sibling;
  return sibling;
}

/* Traverse through each nodes without a recursion.
.. It is the Depth First Search (DFS pre-Order) routine.
.. You can send a function that manipulate the node->data
.. as 'func' arguement of the function.  
*/
Flag TreeEachNode(_Node * node, 
  Flag searchDepth, TreeNodeFunction func){

  if(!node || (searchDepth > TREE_MAX_DEPTH) ) {
    GameError("TreeEachNode() : aborted");
    return 0;
  }

  int level = 0;
  
  /* We iterate through the edges rather than the nodes.
  .. We get the node as , edge->node */
  _Edge root = {.node = node, .sibling = NULL},
    * edge = &root;
  EdgeStack[0] = edge;
 
  while(level >= 0) { 

    /* run the 'func' with the node and go down the tree */ 
    while(level <= searchDepth) {

      /* Do something with node here  */
      if(func && edge->node)
        if(!func(edge->node)){
          GameError("TreeEachNode() : func() failed");
          return 0;
        }
      /* End of "Do something with node here"*/  

      /*Cannot go further down */     
      if(!edge->node || (level == searchDepth) )
        break;
  
      /* Go to child */
      _Edge * child = EdgeGotoChild(edge, &level);
      if(!child) 
        break;
      edge = child;
    }

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( level >= 0 ) {
      _Edge * sibling = EdgeGotoSibling(edge, &level);
      if(sibling) {
        edge = sibling;
        break;  
      }
      edge = EdgeGotoParent(edge, &level);
    } 
  }

  return 1;
}

/* Traverse through each nodes without a recursion.
.. It is the Depth First Search (DFS post-Order) routine.
.. You can send a function that manipulate the node->data
.. as 'rfunc' arguement of the function.  
*/
Flag TreeEachNodePostOrder(_Node * node, 
  Flag searchDepth, TreeNodeFunction rfunc){

  if(!node || (searchDepth > TREE_MAX_DEPTH) ) {
    GameError("TreeEachNodePostOrder() : aborted");
    return 0;
  }

  int level = 0;
  
  /* We iterate through the edges rather than the nodes.
  .. We get the node as , edge->node */
  _Edge root = {.node = node, .sibling = NULL},
    * edge = &root;
  EdgeStack[0] = edge; 
 
  while(level >= 0) { 

    /* Go to the bottom most node */
    while(level < searchDepth && edge->node) {
      _Edge * child = EdgeGotoChild(edge, &level);
      if(!child) 
        break;
      edge = child;
    }

    while ( level >= 0 ) {
      /* Do something with node here  */
      if(rfunc && edge->node) {
        if(!rfunc(edge->node)) {
          GameError("TreeEachNodePostOrder() : rfunc() failed");
          return 0;
        }
      }
      /* End of "Do something with node here"*/  
    
      /* Going to sibling (if any more left to traverse) 
      .. or go to parent */
      _Edge * sibling = EdgeGotoSibling(edge, &level);
      if(sibling) {
        edge = sibling;
        break;  
      }
      edge = EdgeGotoParent(edge, &level);
    } 
  }

  return 1;
}

enum TREE_NODE_FLAG {
  NODE_LEAF = 1,
  NODE_ROOT = 2,
  NODE_PRUNED = 4,
  NODE_PARTIALLY_PRUNED = 8
};

/* NOTE: if using this array 
.. switch off multithreading if any */
Array MOVES_ARRAY = {.p = NULL, .len = 0, .max = 0};

_Edge * EdgeNew() {
  _Edge * edge = EdgeFromPool();
  edge->data   = MoveFromPool();
}

_Mempool * NODE_POOL  = NULL;
_Mempool * EDGE_POOL  = NULL;

Flag TreePoolInit(){
  /* 1 MB each for sets of nodes and edges */
  EDGE_POOL = 
    Mempool(sizeof(_Edge), 1 + 1024*1024/sizeof(_Edge)); 
  NODE_POOL = 
    Mempool(sizeof(_Node), 1 + 1024*1024/sizeof(_Node)); 
  return (EDGE_POOL && NODE_POOL);
}

static inline
_Edge * EdgeNew(_Node * parent, _Move * move) {
  /* New edge */
  _Edge * edge = (_Edge *) MempoolAllocateFrom(NODE_POOL);
  if(!edge) 
    return NULL;
  /* add this edge to the linked list of 
  .. children edges of parent */
  edge->sibling = parent->child;
  parent->child = edge;
  /* End node of edge */
  edge->node = NULL;
  /* Set the move data */
  mempcy(&edge->move, move, sizeof(_Move));

  return edge;
}

static inline
Flag NodeSetEdges(_Node * parent) {
  /*node->child = NULL; */
  if(BoardAllMoves(board, &MOVES_ARRAY) ==  GAME_STATUS_ERROR)
    return 0;
  _Move * move = (_Move *) (MOVES_ARRAY.p);
  Flag nmoves = (Flag) (MOVES_ARRAY.len/sizeof(_Move));
  for(Flag i=0; i<nmoves; ++i, ++move) {
    if(!EdgeNew(parent, move))
      return 0;
  }
}

static inline
_Node * NodeNewRoot(_Board * board) {
  /* New node */
  _Node * node = (_Node *) MempoolAllocateFrom(NODE_POOL);
  if(!node) 
    return NULL;
  /* Set Board */
  mempcy(&node->board, board, sizeof(_Board));
  /* Set flags and counts */
  node->flags = NODE_ROOT|NODE_LEAF;
  node->depth = 0;
  node->nchildren = 0;
  /* First edge */
  node->child = NULL; 
  /* Create edges with all possible moves*/
  if(!NodeSetEdges(node))
    return 0;

  return node;
}

_Node * NodeNewChild(_Node * parent, _Edge * edge) {
  /* Weird! */
  if(edge->node)
    return NULL;
  /* New Node */
  _Node * node = (_Node *) MempoolAllocateFrom(NODE_POOL);
  if(!node) 
    return NULL;
  /* Set Board by inheriting from parent, and then ..
  .. move the board by 'move'. 
  .. WARNING: the status is not yet updated*/
  mempcy(&node->board, &parent->board, sizeof(_Board));
  BoardMove(&node->board, &edge->move);
  /* Set flags and counts */
  node->flags &= ~NODE_LEAF;
  node->flags |= NODE_PARENT;
  node->depth = 0;
  node->nchildren = 0;
  /* First edge child. */ 
  node->child = NULL; 
  /* Create edges with all possible moves*/
  if(!NodeSetEdges(node))
    return 0;
  edge->node = node;

  return node;
}

static inline
Flag NodeFree(_Node * node){
  if(node->depth) {
    /* Only a leaf can be deleted */
    return 0;
  }
  while(node->child) {
    /* Delete edges and the children nodes they are
    .. pointing to */
    _Edge * edge = parent->child;
    parent->child = edge->sibling;
    if(edge->node) 
      return 0;
    MempoolDeallocateTo(EDGE_POOL, edge);
  }
  MempoolDeallocateTo(NODE_POOL, node);
  return 1;
}

Flag NodePrune(_Node * parent) {
  if(parent->depth != 1) {
    /* Only a node with depth 1 can be pruned 
    .. (i.e all children are leaves)
    */
    return 0;
  }
  while(parent->child) {
    /* Delete edges and the children nodes they are
    .. pointing to */
    _Edge * edge = parent->child;
    parent->child = edge->sibling;
    if(edge->node) {
      if(!NodeFree(edge->node))
        return 0;
    }
    MempoolDeallocateTo(EDGE_POOL, edge);
  }
  parent->depth = 0;
  parent->flags &= ~NODE_PARENT;
  parent->flags |= NODE_LEAF|NODE_PRUNED;
  return 1;
}

Flag NodeExpand(_Node * parent) {
  if(parent->depth || parent->child)
    return 0;

  _Board * board = &(parent->board);
  BoardAllMoves( board, &MOVES_ARRAY );
  
  if( board->status == GAME_ERROR ) 
    return 0;

    return ; 
  } else if ( node->depthmax == 0 ){
    node->flags |= IS_PRUNED_NODE;
    return 0; // cannot expand bcs of tree depth limit
  }

  return 1;
}




