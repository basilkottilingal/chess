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

/**
  Copied from "nnue.h"
* Internal piece representation
*     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
*     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12
*
* Make sure the piecesyou pass to the library from your engine
* use this format.
*/
const int MAPPING_NNUE[14] = 
  { 0, 0,
    12, 6, 9, 3, 11, 5,
    10, 4, 8, 2, 7, 1,
  }; //converting to nnue.h format

int GameEvaluate(_Game * g) {
  _GameSquare ** board = g->board;
  int player = !g->color; //NNUE color notation
  int pieces[33], squares[33];
  int ipieces = 0;
  {
    //WKING
    unsigned char s = g->king[WHITE]->square;
    int f = s%8;  
    int r = s/8;
    pieces[ipieces] = MAPPING_NNUE[WKING];
    squares[ipieces] = 8*(7-r) + f; //NNUE sq notation
    ipieces++;
  }
  {
    //BKING
    unsigned char s = g->king[BLACK]->square;
    int f = s%8;  
    int r = s/8;
    pieces[ipieces] = MAPPING_NNUE[BKING];
    squares[ipieces] = 8*(7-r) + f; //NNUE sq notation
    ipieces++;
  }
  for(int i=0; i<8; i++) {
    for(int j=0; j<8; j++) {
      int p = board[i][j].piece;
      if(p == EMPTY || p == WKING || p == BKING)
        continue;
      p = MAPPING_NNUE[p];//convert piece to nnue.h format
      pieces[ipieces] = p;
      squares[ipieces] = 8*(7-i) + j; //NNUE sq notation
      ipieces++;
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
int TreeNodeNegamax(_Tree * node, void * val) {
  if(node->flags & IS_ROOT_NODE) {
    *eval = GameEvaluate(node->g);
    assert(g->
  }
  else 
  
}

_GameMove * EngineMinimax(_Engine * e) {
  _Tree * t = e->tree;
  
}
*/
