/*
ToDo: Naive eval function. MiniMax. a-b pruning.
*/

#include "tree.h"

typedef struct {
  //which color is this Engine representing;
  unsigned char mycolor; 
  //Tree from which you deduce the 'best' moves
  _Tree * tree;
}_Engine;
 
 
//Engine prototype
typedef int (* EngineType)(_Engine * e);

//Not yet assigned which engines represent each colors
EngineType Engines[2] = {NULL, NULL}; 

/*
  A naive evaluation function for a leaf node;
double TreeGameEvalNaive(_TreeNode * node, 
    unsigned char mycolor) {
  assert(node->flags & IS_LEAF_NODE);
}
*/


