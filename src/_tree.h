#include "move.h"
#include "mempool.h"
typedef struct _Node _Node;
typedef struct _Edge _Edge;

/**
          _Node parent;                    @level
        (_Board + flags)
          /      
         /       
        /
  _Edge child; -->  _Edge sibling1; --> _Edge sib2; .. --> NULL;
  (_Move)           (_Move)
      /                \
     /                  \
    /                    \
_Node node0;        _Node node1            @level+1
(_Board, flags)    (_Board, flags)

*/

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

typedef struct {
  /* Root node of the tree */
  _Node * root;
  /* Depth allowed */
  Flag depthmax;
  /* Pool availability at the beginning
  .. before the tree creation */
  size_t nedges, nnodes;
} _Tree;

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
fprintf(stdout, "\n[U %d]",*level); fflush(stdout);

  /* Go up */ 
  if(! (*level)--) 
    /* It applies for root node only. ..
    .. which doesn't have a parent*/
    return NULL;

  /* Pop from the stack. 
  .. Updating depth of parent.
  .. fixme: update flags of parent too*/
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
fprintf(stdout, "->[P%d]",*level); fflush(stdout);
  return parent;
}

static inline
_Edge * EdgeGotoChild(_Edge * edge, int * level) {
fprintf(stdout, "\n[D %d]",*level); fflush(stdout);
  assert(edge->node);
  /* Check if it's a leaf cell */
  if(!edge->node->depth)
    return NULL;
  _Edge * child = edge->node->child;
  if(child)
    /* Go down. Update Stack */
    EdgeStack[++*level] = child;
fprintf(stdout, "->[C%d]",*level); fflush(stdout);
  return child;
}

static inline
_Edge * EdgeGotoSibling(_Edge * edge, int * level) {
fprintf(stdout, "\n[R %d]",*level); fflush(stdout);
  _Edge * sibling = edge->sibling;
  /* Update Stack */
  //if(sibling)
  EdgeStack[*level] = sibling;
fprintf(stdout, "->[S%d]",*level); fflush(stdout);
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
          //GameError("TreeEachNode() : func() failed");
          //return 0;
        }
      /* End of "Do something with node here"*/  

      /* Cannot go further down */     
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
        //fixme: if(sibling->node)
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
          //GameError("TreeEachNodePostOrder() : rfunc() failed");
          //return 0;
        }
      }
      /* End of "Do something with node here"*/  
    
      /* Going to sibling (if any more left to traverse) 
      .. or go to parent */
      _Edge * sibling = EdgeGotoSibling(edge, &level);
      if(sibling) {
        edge = sibling;
        //fixme: if(sibling->node)
        break;  
      }
      edge = EdgeGotoParent(edge, &level);
    } 
  }

  return 1;
}

enum TREE_NODE_FLAG {
  /* This node is pooled from a reserved memory pool. 
  .. Have to be freed before traversing back to parent.*/
  NODE_IS_RESERVED = 128,
  /* Identify the node in the tree */
  NODE_LEAF = 1,
  NODE_PARENT = 2,
  NODE_ROOT = 4,
  /* If node is pruned as it doesn's seems a reasonable move */
  NODE_PRUNED = 8,
  NODE_IS_MYMOVE = 16,
  /* Identifying crtitical moves */
  NODE_IS_SURE_WIN = 32
  /* sure loss = not my move && node is sure win */
};

/* NOTE: if using this array 
.. switch off multithreading if any */
Array MOVES_ARRAY = {.p = NULL, .len = 0, .max = 0};

_Mempool * NODE_POOL  = NULL;
_Mempool * EDGE_POOL  = NULL;

Flag TreePoolInit(){
  /* 1 MB each for sets of nodes and edges */
  if(!EDGE_POOL)  
    EDGE_POOL = 
      Mempool(sizeof(_Edge), 1 + 1024*1024/sizeof(_Edge)); 
  if(!NODE_POOL)  
    NODE_POOL = 
      Mempool(sizeof(_Node), 1 + 1024*1024/sizeof(_Node)); 
  return (EDGE_POOL && NODE_POOL);
}

static inline
Flag NodeSetEdges(_Node * parent) {
  /* First child. Not yet assigned
  .. assert(!parent->child && !parent->nchildren);
  */
  parent->child = NULL;
  parent->nchildren = 0;

  if(BoardAllMoves(&parent->board, &MOVES_ARRAY) ==  GAME_STATUS_ERROR)
    return 0;
  _Move * move = (_Move *) (MOVES_ARRAY.p);
  Flag nmoves = (Flag) (MOVES_ARRAY.len/sizeof(_Move));

  for(Flag i=0; i<nmoves; ++i, ++move) {
    /* New edge */
    _Edge * edge = (_Edge *) MempoolAllocateFrom(EDGE_POOL);
    if(!edge) {
      parent->flags |= NODE_PRUNED;
      return 0;
    }
    /* add this edge to the linked list of 
    .. children edges of parent */
    edge->sibling = parent->child;
    parent->child = edge;
    ++(parent->nchildren);
    /* End node of edge. Not yet assigned*/
    edge->node = NULL;
    /* Set the move data */
    memcpy(&edge->move, move, sizeof(_Move));
  }

  return 1;
}

static inline
_Node * NodeNewRoot(_Board * board) {
  if(board->status != GAME_CONTINUE) {
    GameError("NodeNewRoot() : Game over / Error ");
    return NULL;
  }
  /* New node */
  _Node * node = (_Node *) MempoolAllocateFrom(NODE_POOL);
  if(!node) 
    return NULL;
  /* Set Board */
  memcpy(&node->board, board, sizeof(_Board));
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

_Node * NodeNewLeaf(_Node * parent, _Edge * edge) {
  /* Weird! */
  if(edge->node)
    return NULL;

  /* New Node as the end node of 'edge'*/
  _Node * node = (_Node *) MempoolAllocateFrom(NODE_POOL);
  if(!node) 
    return NULL;
  edge->node = node;

  /* Set Board by inheriting from parent, and then ..
  .. move the board by 'move'. */ 
  _Board * board = &node->board;
  _Move * move = &edge->move;
  memcpy(board, &parent->board, sizeof(_Board));
  BoardMove(board, move);
  if ( BoardUpdateMetadata(board, move) == GAME_STATUS_ERROR ) {
    MempoolDeallocateTo(NODE_POOL, node); 
    return NULL;
  }

  /* Set flags and counts */
  node->flags = NODE_LEAF;
  node->depth = 0;
  node->nchildren = 0;
  /* Create edges with all possible moves*/
  if(!NodeSetEdges(node))
    return 0;

  return node;
}

static inline
Flag NodeFree(_Node * parent){
  if(parent->depth) {
    /* Only a leaf can be deleted */
    return 0;
  }
  while(parent->child) {
    /* Delete edges and the children nodes they are
    .. pointing to */
    _Edge * edge = parent->child;
    parent->child = edge->sibling;
    if(edge->node) 
      return 0;
    MempoolDeallocateTo(EDGE_POOL, edge);
  }
  MempoolDeallocateTo(NODE_POOL, parent);
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
      edge->node = NULL;
    }
  }
  parent->depth = 0;
  parent->flags &= ~NODE_PARENT;
  parent->flags |= NODE_LEAF|NODE_PRUNED;
  return 1;
}

Flag NodeExpand(_Node * parent) {
  /* Weird. Expected a leaf node (i.e depth = 0)*/
  if(parent->depth)
    return 0;

  /* Game Over */
  if(parent->board.status != GAME_CONTINUE)
    return 1;

  /* Weird. Expected all the moves are evaluated*/
  if(!parent->child)
    return 0;

  _Edge * edge = parent->child;
  while(edge) {
    
    /* Weird. Expected the edge's end node not assigned */
    if(edge->node)
      return 0;

    /* Assign a new leaf node with a 'board' and status */
    edge->node = NodeNewLeaf(parent, edge);

    /* Out of memory. So Prune or TreeTraverse without  
    .. saving the tree (Not yet)*/
    if(!edge->node)
      parent->flags |= NODE_PRUNED;

    edge = edge->sibling;
  }

  parent->depth = 1;
  parent->flags &= ~NODE_LEAF;
  parent->flags |= NODE_PARENT;

  return 1;
}


_Tree * Tree(_Board * board, Flag depthmax) {
  if(!board || (depthmax > TREE_MAX_DEPTH)) {
    GameError("Tree() : aborted");
    return NULL;
  }
  if(!TreePoolInit()) {
    GameError("Tree() : Pool cannot be created.");
    return NULL;
  }
  _Node * root = NodeNewRoot(board);
  if(!root) {
    GameError("Tree() : Cannot Create root");
    return NULL;
  }
  _Tree * tree = (_Tree *) malloc(sizeof(_Tree)); 
  tree->root = root;
  tree->depthmax = depthmax;
  tree->nnodes = NODE_POOL->nfree;
  tree->nedges = EDGE_POOL->nfree;

  /* Create tree to maxdepth */
  TreeEachNode(root, depthmax-1, NodeExpand);  

  return tree;
}

Flag TreeDestroy(_Tree * tree) {
  size_t nnodes = tree->nnodes, nedges = tree->nedges;

  /* Prune entire tree */
  TreeEachNodePostOrder(tree->root, tree->depthmax, NodePrune);
  NodeFree(tree->root);
  free(tree);

  /*Stats of pool to verify */
  fprintf(stdout, "\nPool free nodes before %ld, now %ld",
    nnodes, NODE_POOL->nfree);
  fprintf(stdout, "\nPool free edges before %ld, now %ld",
    nedges, EDGE_POOL->nfree);
  return 1;
}

