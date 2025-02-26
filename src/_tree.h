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
  _Node * next;  
  /* Next Edge of this edge's parent */
  struct _Edge * next;
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
_Node * NodeStack[TREE_MAX_DEPTH];

void TreeEachNode(_Node * node, 
  Flag searchDepth, TreeNodeFunction func){

  if(!node) {
    GameError("TreeEachNode() : No root");
    return;
  }

  Flag level = 0;
  NodeStack[0] = NULL;
 
  while(level >= 0) { 

    while(level <= searchDepth) {
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
        node = node->child;
        NodeStack[level] = node->children;
        //Update Depth-max everytime;
        node->depthmax = node->parent->depthmax - 1;
      } 
    }

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( level >= 0 ) {
      //Update depth everytime.
      TreeUpdateParentDepth(node);
      ++(stack[level]); 
      if( *(stack[level]) ) {  
        node = *(stack[level]); 
        //Update Depth-max everytime;
        node->depthmax = node->parent->depthmax - 1;
        break;
      }
      node = node->parent;
      --level;
    } 
  }

  free(rootParent);
}


void TreeEachNodePostOrder(_Tree * root, Flag searchDepth, 
    TreeNodeFunction rfunc){

  assert(root);                                 
  //depth = depth + node->level; //in case node is not the root
  assert(searchDepth <= root->depthmax);               
  _Tree * node = root, ** stack[TREE_MAX_DEPTH + 1],
    ** rootParent = (_Tree **) malloc(2*sizeof(_Tree *));
  rootParent[0] = root;   rootParent[1] = NULL;
  int level = 0;

  //i don;t like it
  stack[level] = rootParent; 
 
  // fixme: make it general for any subtree (even node!=root)
  // while(TREE_STACK[node->level] >= 0) 
  while(level >= 0) { 

    while(level <= searchDepth) {
      
      /* Going down the tree */
      if(node->depth == 0) {  //Cannot go down further
        assert(node->children == NULL);
        assert(node->flags & IS_LEAF_NODE);
        break; 
      }
      else if(level <= searchDepth) {
        //Go to 1st child if node->level is not yet "depth"
        stack[++level] = node->children;
        node = node->children[0];
        //Update Depth-max everytime;
        node->depthmax = node->parent->depthmax - 1;
      } 
    }

    /* Going to sibling (if any more left to traverse) or ..
    .. Go to parent's sibling (if any more left ) or .. */
    while ( level >= 0 ) {
      /* Do something with node here  */
      if(rfunc)
        rfunc(node);
      /* End of "Do something with node here"*/  

      //Update depth everytime.
      TreeUpdateParentDepth(node);
      ++(stack[level]); 
      if( *(stack[level]) ) {  
        node = *(stack[level]); 
        //Update Depth-max everytime;
        node->depthmax = node->parent->depthmax - 1;
        break;
      }
      node = node->parent;
      --level;
    } 
  }

  free(rootParent);
}
