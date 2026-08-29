#ifndef _CHESS_TREE_H_
#define _CHESS_TREE_H_

  #include "move.h"
  #include "eval.h"

  int BoardPickMove (_Board * b, int16_t * score)
  {
    /*
    .. pick "best" move among the remaining moves.
    .. preference : terminal node > TT look up > argmin{eval(move)}
    */

    #define SWAP(at)  do            \
      {                             \
        temp = moves [start];       \
        moves [start] = moves [at]; \
        moves [at] = temp;          \
      } while (0)

    int16_t bestAt = -1, bestScore = INT16_MAX, eval;

    /* assumes moves are listed in the "movesall" array */
    _Move * moves = MOVES_AT (b), temp;
    uint8_t n = b[0].totalMoves, start = 0;
    while (n-- > start)
    {

      if (moves[n].flags == MOVE_ILLEGAL)
      {
        /* stack illegal moves at the beginning of the array */
        SWAP (n);

        start ++;
        n ++;

        /*
        .. adjust [moveLoc, moveLoc+totalMoves) so that you don't have to
        .. traverse this (illegal) move again
        */
        b [0].moveLoc ++;
        b [0].totalMoves --;

        continue;
      }

      _Move * move = & moves [n];
      uint8_t status = BoardRoll (b, move);

      /* this move is picked as the resulting board is a terminal node */
      if ( !GAME_CONTINUES (status) )
      {
        *score = (status & GAME_IS_A_DRAW) ? 0 :
          (status & GAME_WHO_WINS) == WHITE ? 10000 : -10000; 
        SWAP (n);
        return 1;
      }

      /* look in the transposition table */

      /* use some sort of evaluation for this board. */
      eval = BoardEval (b+1);

      if (eval < bestScore) /* fixme */
      {
        bestScore = eval;
        bestAt = n;         
      }

      /* undo the move */
      BoardUnroll (move);

    }

    if (bestAt == -1)
      return 0;

    *score = bestScore;
    SWAP (bestAt);

    _Move * move = & moves [start];
    BoardRoll (b, move);

    return 1;
  }

  int BoardNextLevel (_Board ** y)
  {
    _Board * b = *y;
    int16_t score;
    if ( !BoardPickMove (b, &score) )
      return 0;
    (*y)++;
    return 1;
    /* note that board is already moved and b+1 is update */
  }

  int BoardPrevLevel (_Board ** y)
  {
    _Board * b = --(*y);
    _Move * move = MOVES_AT (b);

    BoardUnroll (move);

    b [0].moveLoc ++;
    b [0].totalMoves --;

    return 1;
  }

  /*
  .. since we avoid recursive functions for tree searching, we need to
  .. maintain a stack of reduction variables like score, alpha, beta, etc
  ..
  */
  typedef struct
  {
    int16_t  bestScore;
    uint16_t bestMoveAt;
    uint64_t stats;
  } Probe;
  Probe ProbePlies [ MAX_STACK_SIZE ];

  #ifdef _CHESS_DEBUG_
    _Move bestMoves[10][10];

    #define bestmove(depth) do                                       \
      {                                                              \
        assert (depth <10);                                          \
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
        if ( !BoardPickMove (b, & score) )
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
      score = -ply->bestScore;
      if (depth == depthmax)
        break;
      b--, ply++, depth ++;

      /* reduction of parent nodes alpha/beta from min{} / max{} of children */
      if (score > ply->bestScore)
      {
        ply->bestScore  = score;
        ply->bestMoveAt = b->moveLoc;
        #ifdef _CHESS_DEBUG_
        bestmove (depth);
        #endif
      }

      /* unroll the board as you popped one ply */
      BoardUnroll (MOVES_AT (b));

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
