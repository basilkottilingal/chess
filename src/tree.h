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
  //unsigned int id; 
  _Board * b;

  Flag level; // level \in [0,8) ??
  Flag flags; // identify type of node
 
  int eval;   //board evaluation value \in [-5000, 5000]

  //Graph Connection using pointers
  Flag nchildren;
  struct _Tree * root;     //this node
  struct _Tree * parent;   //parent node
  struct _Tree * children; //children
} _TreeNode;

#define TREE_MAX_DEPTH 4
#define IS_LEAF_NODE   1
#define IS_ROOT_NODE   2
#define IS_PARENT_NODE 4
#define ARE_ALL_CHILDREN_LEAVES 8
#define IS_NODE_ORIGINAL 16
#define IS_PRUNED_NODE 32

/* ---------------------------------------------------------
------------------------------------------------------------
  Assigne values to a _TreeNode object.
  (NOTE Doesn't create an object here).
------------------------------------------------------------
--------------------------------------------------------- */

void TreeNode(_TreeNode * node, _TreeNode * parent, 
    _Board * b, _BoardMove * move) {

  assert(b->status == GAME_CONTINUE);
  node->b = Board();
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
/* ---------------------------------------------------------
------------------------------------------------------------
  Free Memory associated with a Node 
  (NOTE: But not the node itself)
------------------------------------------------------------
--------------------------------------------------------- */

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

/* ---------------------------------------------------------
------------------------------------------------------------
  Create children for a _TreeNode by expanding using all ..
  .. possible moves.
------------------------------------------------------------
--------------------------------------------------------- */

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
  _TreeNode * child = 
    (_TreeNode *) malloc (nmoves * sizeof (_TreeNode)); 
  node->children = child;

  for (int i=0; i<nmoves; ++i, ++child, ++move) 
    TreeNode(child, node, node->g, move);

  //node is not a leaf anymore;
  node->flags &= ~(IS_LEAF_NODE | IS_PRUNED_NODE);
  node->flags |=  (IS_PARENT_NODE | ARE_ALL_CHILDREN_LEAVES);
  //Not all children of node->parent are leaf anymore
  if(node->parent) 
    node->parent->flags &= ~ARE_ALL_CHILDREN_LEAVES;
  
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
  if( !(node->flags & IS_PARENT_NODE) ) 
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

/* ---------------------------------------------------------
------------------------------------------------------------
  _Tree is the linked list of hierarchical nodes,
  .. starting with a roor node.
------------------------------------------------------------
--------------------------------------------------------- */

typedef struct {
  //Usually used to store current state of the game
  _TreeNode * root;    
  //max depth of search (further from current state)
  unsigned char depth, depthmax;
} _Tree;


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

unsigned char TREE_STACK[TREE_MAX_DEPTH];
typedef int (* TreeNodeFunction) (_TreeNode *); 

void TreeEachNode(_Tree * tree, unsigned char depth, 
    TreeNodeFunction func){

  assert(tree);                                 
  assert(depth <= tree->depthmax);               
  _TreeNode * node = tree->root;             
  TREE_STACK[0] = 1; 
  while(node) {                                 
    while(node->level <= depth) {
      /* Do something with node here  */
      if(func)
        func(node);
      /* End of "Do something with node here"*/  
      
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
      if( --TREE_STACK[node->level] > 0 ) {  
        ++node; 
        break;
      }
      node = node->parent;
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
      node = node->parent;
    } 
  }
}

/* ---------------------------------------------------------
------------------------------------------------------------
  Create a tree from a given instance of game 
------------------------------------------------------------
--------------------------------------------------------- */

_Tree * Tree(_Game * g, unsigned char depth) {
  //srand(time(0));  
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
  TreeEachNode(tree, TREE_MAX_DEPTH-1, TreeNodeExpand);
  // There is no pruning, .. thus around 20^TREE_MAX_DEPTH 
  // is the number of leaf nodes. Make sure you don't ..
  // .. run out RAM and crash the system.
  tree->depth = tree->depthmax;
 
  return tree;
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

  TreeNodeDestroy(tree->root);
  free(tree->root);
  free(tree);
}
