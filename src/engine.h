#include "move.h"
#define NNUE
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
