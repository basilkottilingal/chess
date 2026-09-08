#ifndef _CHESS_TREE_H_
#define _CHESS_TREE_H_

  #include "move.h"
  #include "eval.h"

  #define NO_LEGAL_MOVES       0
  #define A_LEGAL_MOVE(b)    ( (b)->status |= GAME_CONTINUE )

  #define EVAL_DRAW            0
  #define EVAL_LOST(depth)   ( INT16_MIN + 256 - depth )
  #define EVAL_MIN           ( INT16_MIN + 1 )

  /* associated with TT entry score */
  enum
  {
    UPPERBOUND = 1,
    EXACT,
    LOWERBOUND,
  }; 

  /* replace this with quiescence search. fixme : use NNUE/SEE   */
  int16_t quiescence (_Board * b, int depth)
  {
    assert (depth <= 0);

    if (b->status & GAME_IS_A_DRAW)
      return EVAL_DRAW;

    _Move * move = MOVES_AT (b);
    for (int i=0; i<b->totalMoves; ++i, move++)
    {
      BoardMove (b++, move);
      /* there is a legal move */
      if ( !BoardIsKingAttacked (color) )
      {
        BoardUnmove (b--);
        A_LEGAL_MOVE (b);
        return BoardEval (b);
      }
      BoardUnmove (b--);
    }

    /* lost or stalemate in case of no legal move */
    if (b->status & GAME_ON_CHECK)
    {
      b->status = GAME_IS_A_WIN | !color;
      return EVAL_LOST (depth);
    }
    b->status = GAME_IS_A_DRAW | GAME_STALEMATE;
    return EVAL_DRAW;
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

    b->status = 
      (b->halfclock > 99) ? (GAME_IS_A_DRAW | GAME_FIFTY_MOVES)  :
      npieces == 0        ? (GAME_IS_A_DRAW | GAME_INSUFFICIENT) :
      three_fold (b)      ? (GAME_IS_A_DRAW | GAME_THREE_FOLD)   : 
      BoardIsKingAttacked (color) ?
                            (NO_LEGAL_MOVES | GAME_ON_CHECK)     :
                             NO_LEGAL_MOVES ;

    if (b->status & GAME_IS_A_DRAW)
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

  static inline
  int pick_a_move (_Board * b)
  {
    if (!b->totalMoves)
      return 0;

    _Move * move = MOVES_AT (b);

    uint8_t n = b->totalMoves;
    while (n--)
    {
      /*
      .. adjust [moveLoc, moveLoc+totalMoves) so that this move is no more
      .. available
      */
      b->totalMoves --;
      b->moveLoc ++;

      BoardMove (b++, move);
      if (!BoardIsKingAttacked (color))
      {
        A_LEGAL_MOVE (b-1);
        
        HashReinit  (b-1, move);
        if (move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
          npieces--;
        fullclock++;
        color = !color;
        moves_all (b);

        return 1; 
      }
      BoardUnmove (b--);

      move ++;

    }

    if ( ! (b->status & GAME_CONTINUE) )
      b->status =
        (b->status & GAME_ON_CHECK) ?
          (GAME_IS_A_WIN  | !color) :
          (GAME_IS_A_DRAW | GAME_STALEMATE);

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
    _Entry * entry;
  } Probe;
  Probe ProbePlies [ MAX_STACK_SIZE ];

  #ifdef _CHESS_DEBUG_
    _Move bestMoves[MAX_STACK_SIZE][MAX_STACK_SIZE];

    #define bestmove(depth)                                          \
      do                                                             \
      {                                                              \
        bestMoves [depth][depth] =                                   \
          ((_Move * ) movesall.p)[ply->bestMoveAt];                  \
        for (unsigned d = depth-1; d; d--)                           \
          bestMoves [depth][d] = bestMoves [depth-1][d];             \
      } while (0)
  #endif

  /* find best move using alpha-beta pruning */
  _Move * BoardProbeAlphaBeta (const unsigned depthmax)
  {
    assert (depthmax < MAX_STACK_SIZE);

    _Board * b = BoardStack;
    int depth = (int) depthmax;
    memset (ProbePlies, 0, sizeof (ProbePlies));
    Probe * ply = & ProbePlies [depthmax],
      * const max = ply;

    ply->bestMoveAt = UINT16_MAX;
    ply->bestScore  = EVAL_MIN;
    ply->alpha      = EVAL_MIN;
    ply->beta       = -EVAL_MIN;

    /*
    .. stack equivalent of recursive search routine for searching the best
    .. sequence of moves rooted about "b"
    */
    do {

      /* push (until you hit the limit or TT hit or leaf node or beta cutoff)*/
      do
      {
        if ( !pick_a_move (b) )
        {
          if ( b->status & GAME_CONTINUE )
            assert (ply->bestScore > EVAL_MIN);
          else
            ply->bestScore =
              (b->status & GAME_IS_A_WIN) ? EVAL_LOST (depth) : EVAL_DRAW;
          break;
        }

        /* push */
        b++, ply--, depth--;

        ply->stats ++;

        /* inherit alpha-beta bound from parent*/
        ply->alpha = - (ply+1)->beta,
        ply->beta  = - (ply+1)->alpha;

        /* do the TT look up */
        #if 0
        _Entry * e = HashLoc (b->zobrist);
        ply->bestScore  =
          depth == 0 ? quiescence (b, depth) :
          e->hash != b->zobrist ? EVAL_MIN   :
          e->flag   
        break;
        #endif

        ply->bestScore  = depth > 0 ? EVAL_MIN : quiescence (b, depth);
        ply->bestMoveAt = UINT16_MAX;

      } while (depth);
          

      /*
      .. add to TT, if current search rooted with this node is better than
      .. the existing TT entry
      */

      if (depth == depthmax)
        break;

      /* pop */

      int16_t score = -ply->bestScore;
      BoardUnroll (b--);
      ply++, depth ++;
 
      /* reduction of parent node's bestScore from max{-bestScore(child)} */
      if (score > ply->bestScore)
      {
        ply->bestScore  = score;
        ply->bestMoveAt = b->moveLoc - 1;

        if (score > ply->alpha)
          ply->alpha = score;

        #ifdef _CHESS_DEBUG_
        bestmove (depth);
        #endif
      }

      /* fail-soft beta cutoff : discard rest of the child nodes */
      if (score > ply->beta)
        b->totalMoves = 0;

    } while (1);

    assert (depth == depthmax && ply == max);

    if (ply->bestMoveAt == UINT16_MAX)
    {
      assert (b->status & (GAME_IS_A_DRAW | GAME_IS_A_WIN));
      fprintf (stderr, "warning : game over. can't probe.");
      BoardStatusPrint (b);
      return NULL;
    }

    return & ((_Move * ) movesall.p) [ply->bestMoveAt];
  }

  _Move * BoardProbe (const unsigned depthmax)
  {
    assert (depthmax < MAX_STACK_SIZE);

    _Board * b = BoardStack;
    int depth = (int) depthmax;
    memset (ProbePlies, 0, sizeof (ProbePlies));
    Probe * ply = & ProbePlies [depthmax],
      * const max = ply;

    ply->bestMoveAt = UINT16_MAX;
    ply->bestScore  = EVAL_MIN;

    /*
    .. stack equivalent of recursive search routine for searching the best
    .. sequence of moves rooted about "b"
    */
    do {

      /* push (until you hit the limit or TT hit or leaf node or beta cutoff)*/
      do
      {
        if ( !pick_a_move (b) )
        {
          if ( b->status & GAME_CONTINUE )
            assert (ply->bestScore > EVAL_MIN);
          else
            ply->bestScore =
              (b->status & GAME_IS_A_WIN) ? EVAL_LOST (depth) : EVAL_DRAW;
          break;
        }

        b++, ply--, depth--;

        ply->stats ++;

        ply->bestScore  = depth > 0 ? EVAL_MIN : quiescence (b, depth);
        ply->bestMoveAt = UINT16_MAX;
                                                 /* fixme : use NNUE/SEE   */
      } while (depth);

      /* pop (one ply and go to the sibling (next move) of the parent) */
      if (depth == depthmax)
        break;

      int16_t score = -ply->bestScore;
      BoardUnroll (b--);
      ply++, depth ++;
 
      /* reduction */
      if (score > ply->bestScore)
      {
        ply->bestScore  = score;
        ply->bestMoveAt = b->moveLoc - 1;

        #ifdef _CHESS_DEBUG_
        bestmove (depth);
        #endif
      }

    } while (1);

    assert (depth == depthmax && ply == max);

    if (ply->bestMoveAt == UINT16_MAX)
    {
      assert (b->status & (GAME_IS_A_DRAW | GAME_IS_A_WIN));
      fprintf (stderr, "warning : game over. can't probe.");
      BoardStatusPrint (b);
      return NULL;
    }

    return & ((_Move * ) movesall.p) [ply->bestMoveAt];
  }

#endif
