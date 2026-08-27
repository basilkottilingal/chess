#ifndef _CHESS_TREE_H_
#define _CHESS_TREE_H_

  #include "move.h"
  #include "eval.h"

  const uint64_t * const zobrist_castling = & zobrist [64 * 12];
  const uint64_t * const zobrist_enp      = & zobrist [64 * 12 + 16];
  #define          zobrist_color     zobrist [792]
  #define          zobrist_sp(sq,p)  zobrist [(sq) * 12 + (p)]
  #define          zobrist_at(sq)    zobrist [(sq) * 12 + PIECES [sq]]

  /*
  .. A 16 bytes struct that hold zobrist hash, evaluated score, best move 
  .. (6 bits each for start & end and 4 bits for promotion {q,r,b,r}), search
  .. depth and flag
  */
  typedef struct
  {
    uint64_t hash; 
    int32_t  score;
    uint16_t best;
    uint8_t  depth, flag;
  } _Entry;

  /* transposition table for lookup */
  #ifndef TT_SIZE
    #define TT_SIZE                 (1 << 10)
  #endif
  _Entry TT [ TT_SIZE ] = {0};

  #define TT_MOD                    (TT_SIZE - 1)
  #define TT_SEARCH(b)              & TT [b->zobrist & TT_MOD]

  _Entry * HashLoc(uint64_t z)
  {
    return & TT [z & TT_MOD];
  }

  void HashInit ()
  {
    _Board * b = BoardStack;
    uint64_t z = color ? zobrist_color : 0ull;
    for (int i=0; i<64; ++i)
      z ^= zobrist_at (i);
    z ^= zobrist_castling [b->castling];
    if (b->enpassante != OUTSIDE)
      z ^= zobrist_enp [b->enpassante % 8];

    b->zobrist = z;
    memset (TT, -1, sizeof (TT));
  }

  void HashReinit (_Board * b, _Move * m)
  {
    /* warning : call this function only after BoardMove() */
    uint8_t from = m->from.square,
      to = m->to.square, piece = m->from.piece;
    uint64_t *z = &b[1].zobrist;

    *z ^=
      zobrist_color ^                    /* flip side                        */
      zobrist_at (from) ^                /* 'piece' moved from 'from'        */
      zobrist_at (to) ^                  /* 'piece'/'promotion' reached 'to' */
      zobrist_castling [b[0].castling] ^ /* old castling right               */
      zobrist_castling [b[1].castling];  /* new castling right               */

    if (b [0].enpassante != OUTSIDE)
      *z ^= zobrist_enp [b [0].enpassante % 8];
    if (b [1].enpassante != OUTSIDE)
      *z ^= zobrist_enp [b [1].enpassante % 8];
    if (m->flags == MOVE_NORMAL || m->flags == MOVE_PROMOTION)
      return;
    if (m->flags & MOVE_CAPTURE)
    {
      *z ^= zobrist_sp (to, m->to.piece);       /* in case of capture       */
      return;
    }
    if (m->flags & MOVE_ENP_CAPTURE)            /* enp pawn capture         */
    {
      *z ^=
        zobrist_sp (
          to + (piece == WPAWN ? 8 : -8),
          piece == WPAWN ? BPAWN : WPAWN
        );
      return;
    }
    if (m->flags & CASTLING_WQ) /* castling cases & respective rook mov     */
    {
      *z ^= zobrist_sp (a1, WROOK) ^ zobrist_sp (d1, WROOK); 
      return;
    }
    if (m->flags & CASTLING_WK)
    {
      *z ^= zobrist_sp (h1, WROOK) ^ zobrist_sp (f1, WROOK); 
      return;
    }
    if (m->flags & CASTLING_BQ)
    {
      *z ^= zobrist_sp (a8, BROOK) ^ zobrist_sp (d8, BROOK); 
      return;
    }
    if (m->flags & CASTLING_BK)
    {
      *z ^= zobrist_sp (h8, BROOK) ^ zobrist_sp (f8, BROOK); 
      return;
    }
    assert (0);
  }

  _Board * BoardRoot (const char * fen)
  {
    _Board * r = BoardSetFromFEN (fen);
    HashInit ();
    BoardAllMoves (r);
    return r; 
  }

  int BoardPickMove (_Board * b, int16_t * score)
  {
    /*
    .. pick "best" move among remaining moves.
    .. preference : terminal node > TT look up > argmin{eval(move)}
    */

    #define SWAP(at)  do            \
      {                             \
        temp = moves [start];       \
        moves [start] = moves [at]; \
        moves [at] = temp;          \
      } while (0)

    int16_t bestAt = -1, bestScore = INT16_MAX, eval;

    /* assumes moves are listed */
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

      /* move the board */
      _Move * move = & moves [n];
      BoardMove   (b, move);
      HashReinit  (b, move);
      FINISH_MOVE (move);
      uint8_t status = BoardAllMoves (b+1);

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
      FINISH_UNMOVE (move);
      BoardUnmove (move);

    }

    if (bestAt == -1)
      return 0;

    *score = bestScore;
    SWAP (bestAt);

    _Move * move = & moves [start];
    BoardMove   (b, move);
    HashReinit  (b, move);
    FINISH_MOVE (move);
    BoardAllMoves (b+1);

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

    FINISH_UNMOVE (move);
    BoardUnmove (move);

    b [0].moveLoc ++;
    b [0].totalMoves --;

    return 1;
  }

  /*
  .. since we avoid recursive functions for tree searching, we need
  .. to store reduction variables like score, alpha, beta, etc 
  */
  typedef struct
  {
    int16_t  bestScore;
    uint16_t bestMoveAt;
    uint64_t stats;
  } Probe;
  Probe ProbePlies [ MAX_STACK_SIZE ];

  _Move bestMoves[6][6];
  #define reduce(depth) do {                                  \
    bestMoves [depth][depth] =                                \
      ((_Move * ) movesall.p)[ProbePlies [depth].bestMoveAt];\
    for (unsigned d = depth-1; d; d--)                        \
      bestMoves [depth][d] = bestMoves [depth-1][d];          \
  }while (0)

  /* best move */
  _Move * BoardProbe (const unsigned depthmax)
  {
    assert (depthmax < MAX_STACK_SIZE);

    #define UNKNOWN_BEST_MOVE_AT  UINT16_MAX
    _Board * b = BoardStack;
    int16_t score;
    unsigned depth = depthmax;
    memset (ProbePlies, 0, sizeof (ProbePlies));
    ProbePlies [depth].bestMoveAt = UNKNOWN_BEST_MOVE_AT;
    ProbePlies [depth].bestScore  = INT16_MIN;

    do {
      do
      {
        if ( !BoardPickMove (b, & score) )
          break;

        /* push */
        b ++;
        depth --;
        ProbePlies [depth].bestMoveAt = UNKNOWN_BEST_MOVE_AT;
        ProbePlies [depth].bestScore  =
          !GAME_CONTINUES (b->status) ? score : /* Leaf node. game ended  */
          depth ? INT16_MIN :                   /* Do an iterative search */
                                                /* fixme : add TT lookup  */
          score;                                /* At max search depth.   */
                                                /* fixme : use NNUE/SEE   */

        ProbePlies[depth].stats ++;
      } while (depth);

      /*
      .. add to TT, if current search rooted with this node is better than
      .. the existing TT entry
      */

      /* pop */
      score = -ProbePlies [depth].bestScore;
      if (++depth > depthmax)
        break;
      b --;

      _Move * move = MOVES_AT (b);
      FINISH_UNMOVE (move);
      BoardUnmove (move);
      if (score > ProbePlies [depth].bestScore)
      {
        ProbePlies [depth].bestScore  = score;
        ProbePlies [depth].bestMoveAt = b->moveLoc;
reduce (depth);
      }
      b->moveLoc ++;
      b->totalMoves --;

    } while (1);

    assert (depth == depthmax + 1);
    assert (ProbePlies [depthmax].bestMoveAt != UNKNOWN_BEST_MOVE_AT);
    return & ((_Move * ) movesall.p) [ProbePlies [depthmax].bestMoveAt];
  }

#endif
