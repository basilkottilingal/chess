#define NNUE
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
typedef _GameMove * (* EngineType)(_Engine * e);

//Not yet assigned which engines represent each colors
EngineType Engines[2] = {NULL, NULL}; 

/*----------------------------------------------
------------------------------------------------
  Evaluate the board.
------------------------------------------------
------------------------------------------------*/

#ifdef NNUE
#include <stdint.h>
#include "./nnue/nnue.h"

void init_nnue(const char * filename) {
  nnue_init(filename);
}

const int MAPPING_NNUE[14] = 
  { 0, 0,
    12, 6, 9, 3, 11, 5,
    10, 4, 8, 2, 7, 1,
  }; //converting to nnue.h format

#define ENGINE_EVAL_MAX 5000
int EngineOverRideNNUE(_Game * g, int * eval) {
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

int GameEvaluate(_Game * g) {
  int Eval = 0;
  if(g->status)
    if(EngineOverRideNNUE(g, &Eval))
      return Eval;

  _GameSquare ** board = g->board;
  int player = !g->color; //NNUE color notation
  int pieces[33], squares[33];
  int ipieces = 2;
  for(int i=0; i<8; i++) {
    for(int j=0; j<8; j++) {
      int p = board[i][j].piece;
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

int GameEvaluateFEN(_Game * g) {
  return nnue_evaluate_fen(g->fen);
}
#else
/*
double TreeGameEvalNaive(_TreeNode * node, 
    unsigned char mycolor) {
  assert(node->flags & IS_LEAF_NODE);
}
*/
int GameEvaluate(_Game * g) {
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

int TreeNodeNegamax(_TreeNode * node) {
  if(node->flags & IS_ROOT_NODE) {
    return 0; // would be already set by children
  }
  else if(node->flags & IS_LEAF_NODE) {
    node->eval = GameEvaluate(node->g);
  }
  int eval = node->eval;
  _TreeNode * parent = node->parent;
  //assert(parent);
  /*
  if(node == parent->children) { //it's parent's 1st child
    parent->eval = 
      (-ENGINE_EVAL_MAX > -eval) ? -eval 
    parent->move = (_GameMove *) parent->g->moves->p;
  }
  else {
  }
  */
}

_GameMove * EngineMinimax(_Engine * e) {
  _Tree * t = e->tree;
}

