#ifndef _CHESS_TREE_H_
#define _CHESS_TREE_H_

  #include "move.h"

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

  int BoardNextLevel (_Board ** y)
  {
    _Board * b = * y;

    /* assumes moves are listed */
    _Move * move = MOVES_AT (b);
    while (move->flags == MOVE_ILLEGAL && b[0].totalMoves)
    {
      b [0].moveLoc ++;
      b [0].totalMoves --;
      move ++;
    }

    /* if exhausted moves */
    if (!b[0].totalMoves)
      return 0;

    BoardMove (b, move);
    HashReinit (b, move);
    FINISH_MOVE (move);

    BoardAllMoves (++(*y));

    return 1;
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

#endif
