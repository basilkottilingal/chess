#include "engine.h"
#include "mempool.h"
#include "math.h"
typedef struct _Node _Node;
typedef struct _Edge _Edge;

/**
          _Node parent;                    @level
        (_Board + flags)
          /      
         /       
        /
  _Edge child; -->  _Edge sibling; --> sibling; .. --> NULL;
  (_Move)           (_Move)
      /                \
     /                  \
    /                    \
_Node node;        _Node node;            @level+1
(_Board + flags)  (_Board + flags)

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
  Flag nmoves;
  /* Number of children. */
  Flag nchildren;
  /* For engine 
  double alpha, beta;
  */
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

enum TREE_NODE_FLAG {
  /* Identify the node in the tree */
  NODE_LEAF = 1,
  NODE_PARENT = 2,
  NODE_ROOT = 4,
  /* If node is pruned as it doesn's seems a reasonable move */
  NODE_PRUNED = 8,
  NODE_IS_MYMOVE = 16,
  /* Identifying crtitical moves */
  NODE_IS_SURE_WIN = 32,
  /* sure loss = not my move && node is sure win */
  /* This node is pooled from a reserved memory pool. 
  .. Have to be freed before traversing back to parent.*/
  NODE_RESERVED = 128
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

  Flag status = BoardAllMoves(&parent->board, &MOVES_ARRAY); 
  if( status ==  GAME_STATUS_ERROR )
    return 0;
  _Move * move = (_Move *) (MOVES_ARRAY.p);
  Flag nmoves = (Flag) (MOVES_ARRAY.len/sizeof(_Move));

  for(Flag i=0; i<nmoves; ++i, ++move) {
    /* New edge */
    _Edge * edge = (_Edge *) MempoolAllocateFrom(EDGE_POOL);
    if(!edge) 
      return 0;
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
    /* In case few edges are not created (out of memory) */
    node->flags |= NODE_PRUNED;

  return node;
}

_Node * NodeNewLeaf(_Node * parent, _Edge * edge) {
  /* Skip */
  if(edge->node) {
    //GameError("NodeNewLeaf() : Redefinition"); 
    return edge->node;
  }

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
  if ( BoardUpdateMetadata(board, move) == GAME_STATUS_ERROR ){
    MempoolDeallocateTo(NODE_POOL, node); 
    return NULL;
  }

  /* Set flags and counts */
  node->flags = NODE_LEAF;
  node->depth = 0;
  node->nchildren = 0;
  /* Create edges with all possible moves*/
  if(!NodeSetEdges(node))
    /* In case few edges are not created (out of memory) */
    node->flags |= NODE_PRUNED;
  if(NODE_POOL->nfree < TREE_MAX_DEPTH + 50)
    node->flags |= NODE_RESERVED; 

  return node;
}

static inline
Flag NodeEdgesFree(_Node * parent) {
  while(parent->child) {
    /* Delete edges and the children nodes they are
    .. pointing to */
    _Edge * edge = parent->child;
    if(edge->node)
      return 0;
    parent->child = edge->sibling;
    MempoolDeallocateTo(EDGE_POOL, edge);
  }
  return 1;
}

static inline
_Node * NodeFree(_Node * parent){
  if(parent->depth) {
    /* Only a leaf can be deleted. 
    .. 'parent' is not yet deleted*/
    return parent;
  }
  if(NodeEdgesFree(parent)) {
    /* If all the edges are deleted, 
    .. delete the node too */
    MempoolDeallocateTo(NODE_POOL, parent);
    return NULL;
  }
  /* not yet deleted */
  return parent;
}

_Tree * TreeIterator = NULL;
typedef Flag (* TreeNodeFunction) (_Node * node); 

_Edge * EdgeStack[TREE_MAX_DEPTH];

static inline   
_Edge * EdgeGotoParent(_Edge *edge, int * level) {
  #if 0
    fprintf(stdout, "\n[U %d]",*level); fflush(stdout);
  #endif

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
  Flag count = 0;
  while(child) {
    _Node * node = child->node;
    if(node) {
      if((node->flags & NODE_RESERVED) && (!node->depth)) 
        child->node = NodeFree(node);
      else {
        ++count;
        if(depth < node->depth)
          depth = node->depth;
      }
    }
    child = child->sibling;
  }
  if(!count) {
    parent->node->flags &= ~NODE_PARENT;
    parent->node->flags |= NODE_LEAF|NODE_PRUNED;
    parent->node->depth = 0;
  }
  else
    parent->node->depth = 1 + depth;
  #if 0
    fprintf(stdout, "\n[U %d]",*level); fflush(stdout);
  #endif
  return parent;
}

static inline
_Edge * EdgeGotoChild(_Edge * edge, int * level) {

  #if 0
    fprintf(stdout, "\n[D %d]",*level); fflush(stdout);
    assert(edge->node);
  #endif

  /* Go down the tree. Update Stack */
  _Edge * child = edge->node->child;
  if(child)
    EdgeStack[++*level] = child;

  #if 0
    fprintf(stdout, "->[C%d]",*level); fflush(stdout);
  #endif

  return child;
}

static inline
_Edge * EdgeGotoSibling(_Edge * edge, int * level) {
  #if 0
    fprintf(stdout, "\n[R %d]",*level); fflush(stdout);
  #endif
  /* Goto sibling. Update Stack */
  _Edge * sibling = edge->sibling;
  EdgeStack[*level] = sibling;
  #if 0
    fprintf(stdout, "->[S%d]",*level); fflush(stdout);
  #endif
  return sibling;
}

/* Prune the entire tree starting from root. It is
.. same as DFS Post Order. But doesn't pass any function.
.. Sole purpose is prune down the tree, leaving just the 
.. 'root' */
Flag TreePrune(_Node * root) {
  if(!root->depth) {
    NodeEdgesFree(root);
    return 1;
  }

  _Edge root_edge = {.node = root, .sibling = NULL},
    * edge = &root_edge;
  EdgeStack[0] = edge;
  int level = 0;
 
  while(level >= 0) { 

    /* Go to the bottom most, left most node */
    while(edge->node) {
      _Edge * child = EdgeGotoChild(edge, &level);
      if(!child) 
        break;
      edge = child;
    }

    while ( level >= 0 ) {
    
      /* Do Something with edge->node */
      if(edge->node && level) 
        edge->node = NodeFree(edge->node);
      /* End of "Do something with edge->node"*/

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
  
  /* Also, remove the edges starting from node */  
  NodeEdgesFree(root);

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

  _Tree * tree = (_Tree *) malloc(sizeof(_Tree)); 
  tree->depthmax = depthmax;
  tree->nnodes = NODE_POOL->nfree;
  tree->nedges = EDGE_POOL->nfree;

  _Node * root = NodeNewRoot(board);
  if(!root) {
    free(tree);
    GameError("Tree() : Cannot Create root");
    return NULL;
  }
  tree->root = root;

  /* Create tree to maxdepth.
  TreeEachNode(root, depthmax-1, NodeExpand);  
  */

  return tree;
}

Flag TreeDestroy(_Tree * tree) {
  size_t nnodes = tree->nnodes, 
    nedges = tree->nedges,
    nodes = nnodes - NODE_POOL->nfree, 
    edges = nedges - EDGE_POOL->nfree;
  
  fprintf(stdout, "\nDestroying Tree. depth %d, maxdepth %d", 
    tree->root->depth, tree->depthmax);

  /* Prune entire tree */
  TreePrune(tree->root);
  if(NodeFree(tree->root)) {
    GameError("TreeDestroy() : TreePrune() failed");
    return 0;
  }
  free(tree);

  /*Print Stats of pool to verify */
  fprintf(stdout, "\nnode pool consumed %ld", nodes);
  fprintf(stdout, "\nedge pool consumed %ld", edges);
  fprintf(stdout, 
    "\nnode pool: before creation %ld. after destruction %ld.",
    nnodes, NODE_POOL->nfree);
  fprintf(stdout, 
    "\nedge pool: before creation %ld. after destruction %ld",
    nedges, EDGE_POOL->nfree);
  size_t ledges = nedges - EDGE_POOL->nfree, 
         lnodes = nnodes - NODE_POOL->nfree;
  fprintf(stdout,"\nlost %ld edges, %ld nodes",
    ledges, lnodes);

  if(ledges || lnodes) {
    GameError("TreeDestroy() : Reporting memory leak");
    return 0;
  }

  return 1;
}

_Tree * TreeNext(_Tree * tree, _Edge * edge) {
  if(!(tree && edge)) {
    GameError("TreeNext() : aborted");
    return NULL;
  }
  if(edge->node) {
    _Tree * next = (_Tree *)malloc(sizeof(_Tree));
    memcpy(next, tree, sizeof(_Tree));
    next->root = edge->node;
    next->root->flags |= NODE_ROOT;
    edge->node = NULL;
    TreeDestroy(tree);
    return next;
  }
  TreePrune(tree->root);
  BoardNext(&tree->root->board, &edge->move, &MOVES_ARRAY);
  _Tree * next = Tree(&tree->root->board, tree->depthmax);
  if(!next)
    GameError("TreeNext() : failed");
  TreeDestroy(tree);
  return next;
}

struct AlphaBeta {
  double alpha, beta, val;
};
  
struct AlphaBeta AlphaBetaStack[TREE_MAX_DEPTH];

Flag AlphaBetaPruning(_Edge * edge, int * level) {
  Flag l = (Flag) (*level) - 1;
  if( l & 2 ) {
    /* Minimising player */
    AlphaBetaStack[l].val = 
      fmin(AlphaBetaStack[l].val, AlphaBetaStack[l+1].val);
    AlphaBetaStack[l].beta = 
      fmin(AlphaBetaStack[l].val, AlphaBetaStack[l].beta);
  }
  else {
    /* Maximising player */
    AlphaBetaStack[l].val = 
      fmax(AlphaBetaStack[l].val, AlphaBetaStack[l+1].val);
    AlphaBetaStack[l].alpha = 
      fmax(AlphaBetaStack[l].val, AlphaBetaStack[l].alpha);
  }
  if(AlphaBetaStack[l].alpha >= AlphaBetaStack[l].beta) {
    /* Prune upcoming siblings of 'edge'*/
    _Edge * sibling = edge->sibling;
    while(sibling) {
      if(sibling->node) {
        TreePrune(sibling->node);
        sibling->node = NodeFree(sibling->node);
      }
      sibling = sibling->sibling;
    }
    return 1;
  }

  return 0;
}

/* Alpha Beta pruning */
_Move * TreeAlphaBeta(_Node * root, Flag searchDepth) {

  if(!root || (searchDepth > TREE_MAX_DEPTH) ) {
    GameError("TreeEachNode() : aborted");
    return NULL;
  }

  if(!root->child)
    NodeSetEdges(root);

  size_t EdgeCount = 0, NodeCount = 1;

  /* We iterate through the edges rather than the nodes.
  .. We get the node as , edge->node */
  _Edge root_edge = {.node = root, .sibling = NULL},
    * edge = root->child, * parent = &root_edge;
  AlphaBetaStack[0] = (struct AlphaBeta)
    {.alpha = -ENGINE_EVAL_MAX, .beta = ENGINE_EVAL_MAX, 
     .val = -ENGINE_EVAL_MAX};
  EdgeStack[0] = parent;
  EdgeStack[1] = edge;
  
  int level = 1;
 
  while(level >= 1) { 

    /* run the 'func' with the node and go down the tree */ 
    while(level <= searchDepth) {
      ++EdgeCount;
   
      /* Create edge->node if not available */ 
      _Node * node = NodeNewLeaf(parent->node, edge);
      if(!node) 
        break;
      ++NodeCount;

      /* Inheriting alpha/beta from parent */
      AlphaBetaStack[level] = AlphaBetaStack[level-1];
      AlphaBetaStack[level].val = 
        level&1 ?  /* Odd level: Minimising player */
        ENGINE_EVAL_MAX : -ENGINE_EVAL_MAX;

      /*Prune the part that you won't explore*/     
      if( level == searchDepth ) {
        TreePrune(node);
        break;
      }
      /* If edges are not available, create them */
      if(!node->child)
        NodeSetEdges(node);
  
      /* Go to child */
      _Edge * child = EdgeGotoChild(edge, &level);
      if(!child) 
        break;
      parent = edge;
      edge = child;
    }

    /* Evaluate for leaf node */
    _Node * node = NodeNewLeaf(parent->node, edge);
    if(node)  
      AlphaBetaStack[level].val = NnueEvaluate(&node->board);
    else
      GameError("TreeAlphaBeta() : Leaf node not available");
    

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( level >= 1 ) {

      /* Reduction operation. of alpha, beta */
      if(AlphaBetaPruning(edge, &level)) {
        edge = EdgeGotoParent(edge, &level);
        parent = level ? EdgeStack[level-1] : NULL;
        if(!level) break;
      }

      _Edge * sibling = EdgeGotoSibling(edge, &level);
      if(sibling) {
        edge = sibling;
        break;  
      }
      edge = EdgeGotoParent(edge, &level);
      parent = level ? EdgeStack[level-1] : NULL;
    } 
  }

  assert(!level);

  int val = -ENGINE_EVAL_MAX;
  edge = root->child;
  _Move * move = NULL;
/*
  while(edge) {
    if(edge->node) {
      if(val < edge->node->alpha) {
        val = edge->node->val;
        move = &edge->move;
      }
    }
    edge = edge->sibling;
  }
*/
  fprintf(stdout, "\nTree Statistics .Edges %ld, Nodes %ld",
    EdgeCount, NodeCount);  

  return move;
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
#if 0

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
  
      _Node * node = edge->node;
    
      if(!node) 
        break;

      /* Expand the node, if it's a leaf node node */
      if(!node->depth && level < searchDepth)
        NodeExpand(node);

      /* Do something with node here  */
      if(func)
        func(node);
      /* End of "Do something with node here"*/  

      /* Cannot go further down */     
      if(level == searchDepth) {
        /* Don't need the part of the tree down from here */
        TreePrune(node);
        break;
      }
  
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

    /* Go to the bottom most, left most node */
    while(level < searchDepth && edge->node) {
      /* Expand the node, if it's a leaf node node */
      _Node * node = edge->node;
      if(!node->depth)
        NodeExpand(node);
      /*Go down */
      _Edge * child = EdgeGotoChild(edge, &level);
      if(!child) 
        break;
      edge = child;
    }
      
    /* Prune down the line, where you will never visit */
    if(edge->node) { 
      TreePrune(edge->node); 
      #if 0
      assert(!edge->node->child);
      #endif
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
        //fixme: if(sibling->node)
        break;  
      }
      edge = EdgeGotoParent(edge, &level);
    } 
  }

  return 1;
}
#endif
