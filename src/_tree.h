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
  _Node * end;  
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
static inline   
void TreeUpdateParentDepth(_Node * parent) {
  Flag depth = 0;
  _Edge * e = parent->child;
  while(e) {
    if(e->end)
      depth = (depth < e->end->depth) ? e->end->depth : depth;
    e = e->sibling;
  }
  parent->depth = 1 + depth;
  //return depth;
}

typedef Flag (* TreeNodeFunction) (_Node * node); 
_Edge * _EdgeStack_[TREE_MAX_DEPTH+1];

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

  Flag level = 0;
  
  _Edge root = {.end = node, .sibling = NULL, .data = NULL},
    * edge = &root;
  _Edge ** EdgeStack = &_EdgeStack_[1];
  EdgeStack[-1] = NULL;
  EdgeStack[0] = edge;
 
  while(level >= 0) { 

    while(level <= searchDepth) {
      /* We iterate through the edges rather than the nodes.
      .. We get the node as , edge->end */
      _Node * node = edge->end;
      if(!node)
        break;

      /* Do something with node here  */
      if(func)
        func(node);
      /* End of "Do something with node here"*/  
      
      /* Going down the tree */
      if(node->depth == 0) {  
        /* Cannot Go down further.
        .. fixme: do dynamical tree. i.e tree can,
        .. expand to max level while traversal.*/
        assert(node->child == NULL);
        assert(node->flags & IS_LEAF_NODE);
        break; 
      }
      else if(level <= searchDepth) {
        //Go to 1st child if node->level is not yet "depth"
        assert(node->child);
        edge = node->child;
        EdgeStack[++level] = edge; //edge->sibling;
        //Update Depth-max everytime;
        //node->depthmax = node->parent->depthmax - 1;
      } 
    }

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( level >= 0 ) {
      _Edge * parent = EdgeStack[level - 1];
      //Update depth of parent everytime.
      TreeUpdateParentDepth(parent, edge);
      edge = edge->next;
      if( edge ) {  
        EdgeStack[level] = edge;
        //Update Depth-max everytime;
        //node->depthmax = node->parent->depthmax - 1;
        break;
      }
      edge = parent;
      --level;
    } 
  }

}

/* Traverse through each nodes without a recursion.
.. It is the Depth First Search (DFS post-Order) routine.
.. You can send a function that manipulate the node->data
.. as 'func' arguement of the function.  
*/
void TreeEachNodePostOrder(_Node * node, 
  Flag searchDepth, TreeNodeFunction func){

  if(!node || (searchDepth > TREE_MAX_DEPTH) ) {
    GameError("TreeEachNodePostOrder() : aborted");
    return;
  }

  int level = 0;
  
  _Edge root = {.end = node, .sibling = NULL, .data = NULL},
    * edge = &root;
  EdgeStack[0] = edge;
 
  while(level >= 0) { 

    while(level <= searchDepth) {
      /* We iterate through the edges rather than the nodes.
      .. We get the node as , edge->end */
      _Node * node = edge->end;
      if(!node)
        break;

      
      /* Going down the tree */
      if(node->depth == 0) {  
        /* Cannot Go down further.
        .. fixme: do dynamical tree. i.e tree can,
        .. expand to max level while traversal.*/
        assert(node->child == NULL);
        assert(node->flags & IS_LEAF_NODE);
        break; 
      }
      else if(level <= searchDepth) {
        //Go to 1st child if node->level is not yet "depth"
        assert(node->child);
        edge = node->child;
        EdgeStack[++level] = edge; //edge->sibling;
        //Update Depth-max everytime;
        //node->depthmax = node->parent->depthmax - 1;
      } 
    }

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( level >= 0 ) {
      /* Do something with node here  */
      _Node * node = edge->end;
        if(rfunc && node)
          rfunc(node);
      /* End of "Do something with node here"*/  

      _Edge * parent = EdgeStack[level - 1];
      //Update depth of parent everytime.
      TreeUpdateParentDepth(parent, edge);
      edge = edge->next;
      if( edge ) {  
        EdgeStack[level] = edge;
        //Update Depth-max everytime;
        //node->depthmax = node->parent->depthmax - 1;
        break;
      }
      edge = parent;
      --level;
    } 
  }

}

