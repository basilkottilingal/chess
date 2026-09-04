#ifndef _CHESS_TREE_H_
#define _CHESS_TREE_H_

  #include "move.h"
  #include "eval.h"

  #define STATUS_UNKNOWN   0
  #define FOUND_A_MOVE(b)  (b->status = GAME_CONTINUE)

  /* replace this with quiescence search */
  int16_t quiescence (_Board * b)
  {
    _Move * move = MOVES_AT (b);
    for (int i=0; i<b->totalMoves; ++i)
    {
      BoardMove (b++, move);
      if (!BoardIsKingAttacked (color))
      {
        BoardUnmove (b--);
        return BoardEval (b);
      }
      BoardUnmove (b--);
    }
    return BoardIsKingAttacked (color)   ?
      ( INT16_MIN + 1 )  /* lost */      :
      0                  /* stalemate */ ;
  }

  int moves_all (_Board * b)
  {
    /* the move that resulted this board, is an illegal move */
    assert ( !BoardIsKingAttacked (!color) );

    /*
    .. Update the location in the stack "movesall.p" where you are going to
    .. storing the moves
    */
    b [0].moveLoc    = b [-1].moveLoc + b [-1].totalMoves;
    b [0].totalMoves = 0;
    movesall.len     = b->moveLoc * sizeof (_Move);
    /* assert (movesall.max >= movesall.len); */

    b->status = 
      (b->halfclock > 99) ? (GAME_IS_A_DRAW | GAME_FIFTY_MOVES)  :
      npieces == 0        ? (GAME_IS_A_DRAW | GAME_INSUFFICIENT) :
      /*three_fold (b)    ? (GAME_IS_A_DRAW | GAME_THREE_FOLD)   : */
                            STATUS_UNKNOWN;

    if (b->status & GAME_IS_A_DRAW)
      /* Game is a draw */
      return b->status;

    /* Add all move.coms (incl invalid moves). They are still not marked */ 
    for (int i=START; i<=END; ++i)
    {
      Square from = & BOARD [i][START];
      for(int j=START; j<=END; ++j, ++from)
      {
        if ( IS_EMPTY (from) || PIECE_COLOR (from) != color )
          continue;
        /* Generate possible moves with the 'piece' */
        BoardPieceMoves [PIECE (from)] (b, from, &movesall);
      }
    }

    b->totalMoves = 
      (uint8_t) ((movesall.len/sizeof (_Move)) - b->moveLoc);

    return b->status;

  }

  _Board * board_root (const char * fen)
  {
    _Board * r = BoardSetFromFEN (fen);
    if (r == NULL || BoardIsKingAttacked (!color))
    {
      BoardStack->status = GAME_STATUS_ERROR;
      return NULL;
    }
    r [-1].status = GAME_CONTINUE;
    HashInit (r);
    moves_all (r);
    return r; 
  }

  int pick_a_move (_Board * b, int16_t * score)
  {
    if (!b->totalMoves)
      return 0;

    _Move * move = MOVES_AT (b);
    
    for (uint8_t i = 0; i < b->totalMoves; ++i)
    {
      BoardMove (b, move);
      if (!BoardIsKingAttacked (color))
      {
        FOUND_A_MOVE (b);
        
        HashReinit  (b, move);
        if (move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
          npieces--;
        fullclock++;
        color = !color;
        moves_all (++b);

        return 1; 
      }
      BoardUnmove (move);
      move ++, b->moveLoc ++;
    }

    b->status = BoardIsKingAttacked (color) ?
      ( (b->status == STATUS_UNKNOWN) ? (GAME_IS_A_DRAW | GAME_STALEMATE) : 
        (GAME_CONTINUE | GAME_ON_CHECK) ) :
      ( (b->status == STATUS_UNKNONW) ? (GAME_IS_A_WIN  | !color) : 
        GAME_CONTINUE ) ;

    return 0;
  }
    

  /*
  .. since we avoid recursive functions for tree searching, we need to
  .. maintain a stack of reduction variables like score, alpha, beta, etc
  ..
  */
  typedef struct
  {
    int16_t  bestScore;
    int16_t  alpha, beta;
    uint16_t bestMoveAt;
    uint64_t stats;
  } Probe;
  Probe ProbePlies [ MAX_STACK_SIZE ];

  #ifdef _CHESS_DEBUG_
    _Move bestMoves[MAX_STACK_SIZE][MAX_STACK_SIZE];

    #define bestmove(depth) do                                       \
      {                                                              \
        bestMoves [depth][depth] =                                   \
          ((_Move * ) movesall.p)[ply->bestMoveAt];    \
        for (unsigned d = depth-1; d; d--)                           \
          bestMoves [depth][d] = bestMoves [depth-1][d];             \
      } while (0)
  #endif

  /* best move */
  _Move * BoardProbe (const unsigned depthmax)
  {
    assert (depthmax < MAX_STACK_SIZE);

    #define UNKNOWN_BEST_MOVE_AT  UINT16_MAX
    _Board * b = BoardStack;
    unsigned depth = depthmax;
    memset (ProbePlies, 0, sizeof (ProbePlies));
    Probe
      * const max = & ProbePlies [depthmax],
      * const min = ProbePlies,
      * ply = max;
    int16_t score;

    ply->bestMoveAt = UINT16_MAX;
    ply->bestScore  = INT16_MIN;

    /*
    .. stack equivalent of recursive search routine for searching the best
    .. sequence of moves rooted about "b"
    */
    do {

      /* push (until you hit the limit or TT hit or leaf node or beta cutoff)*/
      do
      {
        if ( !pick_a_move (b) )
          break;

        b++, ply--, depth--;

        ply->stats ++;
        ply->bestMoveAt = UINT16_MAX;
        ply->bestScore  =
          !GAME_CONTINUES (b->status) ? score :  /* Leaf node. game ended  */
          depth ? INT16_MIN :                    /* Do an iterative search */
                                                 /* fixme : add TT lookup  */
          score;                                 /* At max search depth.   */
                                                 /* fixme : use NNUE/SEE   */
      } while (depth);

      /*
      .. add to TT, if current search rooted with this node is better than
      .. the existing TT entry
      */

      /* pop (one ply and go to the sibling (next move) of the parent) */
      if (depth == depthmax)
        break;

      score = -ply->bestScore;
      BoardUnroll (b--);
      ply++, depth ++;

      /* reduction of parent nodes alpha/beta from min{} / max{} of children */
      if (score > ply->bestScore)
      {
        ply->bestScore  = score;
        ply->bestMoveAt = b->moveLoc;
        #ifdef _CHESS_DEBUG_
        bestmove (depth);
        #endif
      }

      /*
      .. adjust [moveLoc, moveLoc+totalMoves) of the parent ply. It will help 
      .. "BoardPickMove ()" to produce the next preferred move
      */
      b->moveLoc ++;
      b->totalMoves --;

    } while (1);

    assert (depth == depthmax && ply == max);
    assert (ply->bestMoveAt != UNKNOWN_BEST_MOVE_AT);

    return & ((_Move * ) movesall.p) [ply->bestMoveAt];
  }

#endif
