#define NNUE
/*
ToDo: Naive eval function. MiniMax. a-b pruning.
*/

#include "tree.h"

typedef struct _Engine{
  //which color is this Engine representing;
  Flag mycolor; 
  //Tree from which you deduce the 'best' moves
  _Tree * tree;
  //fn pointer to update the tree when opponent makes move
  Flag (* update_tree) (struct _Engine * e, _BoardMove * move);
  //By default the tree is also updated.
  Flag (*engine) (struct _Engine * e);
}_Engine;

Flag EngineUpdateTree(_Engine * e, _BoardMove * m){
  _Tree * root = e->tree;

  //if(!root)
  //  return 0;

  _Board * b = &root->board;
  if(b->status != GAME_CONTINUE)
    return 0;

  //if(!(root->nchildren && root->children))
  //  return 0;

  _Tree ** children = root->children;

  for(Flag i=0; i<root->nchildren; ++i) {
    _Tree * child = children[i];

    //if(!child)
    //  return 0;
    if(BoardMoveCompare(m, &child->move)) {

      _Tree * next = TreeNext(e->tree, i);
      if(!next)
        //weird. We expect root node to have all children
        return 0;

      //Old tree will be deleted in TreeNext();
      e->tree = next; 

      return 1;
    }
  }
  
  //couldn't find
  fprintf(stderr, "No move");
  return 0;
}
 
/*----------------------------------------------
------------------------------------------------
  Evaluate the board.
------------------------------------------------
------------------------------------------------*/
#define ENGINE_EVAL_MAX 5000

#ifdef NNUE
#include <stdint.h>
#include <nnue.h>

void NnueInit(const char * filename) {
  nnue_init(filename);
}

const int MAPPING_NNUE[14] = 
  { 0, 0,
    12, 6, 9, 3, 11, 5,
    10, 4, 8, 2, 7, 1,
  }; //converting to nnue.h format

int EngineOverRideNnue(_Board * b, int * eval) {
  NOT_UNUSED(b);
  NOT_UNUSED(eval);
  //assert(tree->depth == 0);
  /* Nnue engines doesn't return +-5000 for a win, ..
     .. or 0 for a stalemate/3Fold/InsuffMaterial/50Moves ..
     .. draw. But when to override?? is a debatable one. We ..
     .. leave it for later.

  unsigned int status = g->status;
  if(status) {
    //Game Over
    assert((status & 16) ^ (status & 128));
    //Only draw/win/lose allowed. Other flags are meaningless

    //Game is a win/lose
    if(status & 16) {
      assert(!(status & 64)); 
      //Otherwise one player ran out of time,
      //That means engine also ran out of time??

      //Eval condition for either wins. 
      //assert((status & 32)^(g->color));
      *eval =  g->color ? -ENGINE_EVAL_MAX :  //white fails
        ENGINE_EVAL_MAX; //black fails
    } 
    //Game is a draw 
    else if (status & 128){
      int draw = (status >> 8) & (1|2|3);
      assert(draw < 4); 
      //i.e stalemate/threefold/InsuffMaterial/50Move
      //Other draw conds doesn't make sense     
      *eval = 0; //Eval for a draw ; 
    }
    return 1;//override 
  }
  */
  return 0;//doesn't override as of now
}

int NnueEvaluate(_Board * b) {
  int Eval = 0;
  if(b->status) 
    //Game is theoretically over. So no need to evaluate ?
    if(EngineOverRideNnue(b, &Eval))
      return Eval;

  Piece * sqpiece = b->pieces;
  int player = !b->color; //NNUE color notation
  int pieces[33], squares[33];
  int ipieces = 2;
  for(int i=0; i<8; i++) {
    for(int j=0; j<8; j++) {
      int p = (int) (*sqpiece);
      ++sqpiece;
      if(p == EMPTY) continue;
      int ip = (p == WKING) ? 0 : (p == BKING) ? 1 : ipieces;
      ipieces += ((p != WKING) && (p!=BKING)) ? 1 : 0;
      p = MAPPING_NNUE[p];//convert piece to "nnue.h" format
      pieces[ip] = p;     //piece as in "nnue.h" format
      squares[ip] = 8*(7-i) + j; //sq notation as in "nnue.h"
    }
  }
  pieces[ipieces] = 0;
  //get the eval
  return nnue_evaluate(player, pieces, squares);
}

int NnueEvaluateFEN(_Board * b) {
  //slow. use only for verificn
  char fen[FEN_MAXSIZE];
  BoardFEN(b,fen);
  return nnue_evaluate_fen(fen);
}
#else
/*
double TreeGameEvalNaive(_Tree * node, 
    unsigned char mycolor) {
  assert(node->flags & IS_LEAF_NODE);
}
*/
int NnueEvaluate(_Board * b) {
  assert(0);//Not yet implemented
}
#endif

/*
NegaMax pseudo algo
int negaMax( int depth ) {
    if ( depth == 0 ) return evaluate();
    int max = -oo;
    for ( all moves)  {
        score = -negaMax( depth - 1 );
        if( score > max )
            max = score;
    }
    return max;
}
*/

Flag TreeReorderMoves(_Tree * node) {
  if(node->flags & IS_LEAF_NODE)
    return 0;

  // Order the children of this node ,
  // .. in accordance of their availability in hashtable,
  // .. killer move, etc.
  // Ordering helps to prune most of the tree.
  return 1; 
}

Flag TreeNodeNegamax(_Tree * node) {
  if(!node->depth)  {
    //FIXME: use/search hashtable. Implement table 1st
    node->eval = NnueEvaluate(&node->board);
  }
  else {
    node->ichild = node->nchildren; //An invalid number
    _Tree ** children = node->children;
    assert(children);
    node->eval = -ENGINE_EVAL_MAX;
    for(int i=0; i<node->nchildren; ++i, ++children) {
      _Tree * child = *children;
      assert(child);
      if( (-child->eval) > node->eval) {  
        node->eval =  -child->eval;
        node->ichild = i;
      } 
    }
    assert(node->ichild < node->nchildren);
  }
  return 1;
}

Flag EngineMinimax(_Engine * e) {
  /* Run the minimax evaluate algo*/
  _Tree * root = e->tree;
    
  TreeEachNodePostOrder(root, root->depth, TreeNodeNegamax);
  //this is the most "ideal" move acc to engine
  //_Tree * next = root->children[root->ichild];

  Flag ichild = root->ichild;

  _Tree * next = TreeNext(root, ichild);
  if(!next) {
    GameError("EngineMinimax : TreeNext() failed");
    return 0;
  }

  e->tree = next;
  
  return 1;
}


_Engine * EngineNew(_Board * board, Flag mycolor) {
  _Tree * tree = Tree(board, TREE_MAX_DEPTH);
  if(!tree) { 
    GameError("EngineNew() : Couldn't create tree");
    return NULL;
  }
  _Engine * e = (_Engine *) malloc (sizeof(_Engine));
  e->tree = tree;
  e->mycolor = mycolor;
  e->update_tree = EngineUpdateTree; 
  e->engine = EngineMinimax;
  return e;
}

void EngineDestroy(_Engine * e){
  TreeDestroy(e->tree);
  free(e);
}
