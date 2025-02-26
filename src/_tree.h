struct _Edge;

typedef struct _Node {
  /* First child */
  struct _Edge * child;
  /* Data for the node */
  void * data;
  /* Flags of the node */
  Flag flags;
  /* Depth of this node */
  Flag depth; 
  /* Number of children. */
  Flag nchildren;
} _Node;

typedef struct _Edge {
  /* End node of this edge */
  _Node * node;  
  /* Next Edge of this edge's parent */
  struct _Edge * sibling;
  /* Data for the edge */
  void * data;
} _Edge;

typedef struct {
  /* Root Node of the tree */
  _Node * root;
  /* Max depth allowed */
  Flag depthmax;
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
  /* It applies for root node only. ..
  .. which doesn't have a parent*/
  if(!*level--) 
    return NULL;

  /* Update the depth of parent 
  .. fixme: update parent's flag too*/
  _Edge * parent = EdgeStack[*level];
  Flag depth = 0;
  _Edge * child = parent->child;
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
void TreeEachNode(_Node * node, 
  Flag searchDepth, TreeNodeFunction func){

  if(!node || (searchDepth > TREE_MAX_DEPTH) ) {
    GameError("TreeEachNode() : aborted");
    return;
  }

  int level = 0;
  
  /* We iterate through the edges rather than the nodes.
  .. We get the node as , edge->node */
  _Edge root = {.node = node, .sibling = NULL, .data = NULL},
    * edge = &root;
 
  while(level >= 0) { 

    /* run the 'func' with the node and go down the tree */ 
    while(level <= searchDepth) {

      /* Do something with node here  */
      if(func && edge->node)
        func(edge->node);
      /* End of "Do something with node here"*/  

      /*Cannot go further down */     
      if(!edge->node || (level == searchDepth) )
        break;
 
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

}

/* Traverse through each nodes without a recursion.
.. It is the Depth First Search (DFS post-Order) routine.
.. You can send a function that manipulate the node->data
.. as 'rfunc' arguement of the function.  
*/
void TreeEachNodePostOrder(_Node * node, 
  Flag searchDepth, TreeNodeFunction rfunc){

  if(!node || (searchDepth > TREE_MAX_DEPTH) ) {
    GameError("TreeEachNodePostOrder() : aborted");
    return;
  }

  int level = 0;
  
  /* We iterate through the edges rather than the nodes.
  .. We get the node as , edge->node */
  _Edge root = {.node = node, .sibling = NULL, .data = NULL},
    * edge = &root;
 
  while(level >= 0) { 

    while(level < searchDepth && edge->node) {
      _Edge * child = EdgeGotoChild(edge, &level);
      if(!child) 
        break;
      edge = child;
    }

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( level >= 0 ) {
      /* Do something with node here  */
      if(rfunc && edge->node)
        rfunc(edge->node);
      /* End of "Do something with node here"*/  
      
      _Edge * sibling = EdgeGotoSibling(edge, &level);
      if(sibling) {
        edge = sibling;
        break;  
      }
      edge = EdgeGotoParent(edge, &level);
    } 
  }
}

enum TREE_NODE_FLAG {
  NODE_LEAF = 1,
  NODE_ROOT = 2,
  NODE_PRUNED = 4,
  NODE_PARTIALLY_PRUNED = 8
};

/* NOTE: if using this array 
.. switch of multithreading if any */
Array MOVES_ARRAY = {.p = NULL, .len = 0, .max = 0};

_Edge * EdgeNew() {
  _Edge * edge = EdgeFromPool();
  edge->data   = MoveFromPool();
}

_Mempool * 

_Node * NodeRoot(_Board * board, _) {
  assert(b->status == GAME_CONTINUE);

  root->depth = 0;
  root->depthmax = depthmax;
  //root->level = 0; 
  root->flags = IS_LEAF_NODE | IS_ROOT_NODE;
  root->nchildren = 0;
  // Tree connection
  root->parent = NULL;
  root->children = NULL;
  // create new board & copy content from 'b'
  //root->board = Board(b); 
  BoardCopy(&root->board, b);
}


