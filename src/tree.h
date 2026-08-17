#ifndef _CHESS_TREE_H_
#define _CHESS_TREE_H_

  #include "move.h"

  void HashInit ()
  {
    _Board * b = BoardStack;
    uint64_t z = color ? ZHCOLOR : 0ull;
    for (int i=0; i<64; ++i)
      z ^= ZOB (i);
    z ^= ZHCASTLING [b->castling];
    if (b->enpassante != OUTSIDE)
      z ^= ZHENP [b->enpassante % 8];
    b->zobrist = z;
  }

  void HashReinit (_Board * b, _Move * m)
  {
    /* warning : call this function only after BoardMove() */
    uint8_t from = m->from.square,
      to = m->to.square, piece = m->from.piece;
    uint64_t *z = &b[1].zobrist;

    *z ^=
      ZHCOLOR ^                         /* flip side                        */
      ZOB (from) ^                      /* 'piece' moved from 'from'        */
      ZOB (to) ^                        /* 'piece'/'promotion' reached 'to' */
      ZHCASTLING [b[0].castling] ^      /* old castling right               */
      ZHCASTLING [b[1].castling];       /* new castling right               */

    if (b [0].enpassante != OUTSIDE)
      *z ^= ZHENP [b [0].enpassante % 8];
    if (b [1].enpassante != OUTSIDE)
      *z ^= ZHENP [b [1].enpassante % 8];
    if (m->flags == MOVE_NORMAL || m->flags == MOVE_PROMOTION)
      return;
    if (m->flags & MOVE_CAPTURE)
    {
      *z ^= zobrist [to * 12 +  m->to.piece];   /* in case of capture       */
      return;
    }
    if (m->flags & MOVE_ENP_CAPTURE)            /* enp pawn capture         */
    {
      *z ^=
        zobrist
          [ 
            (to + (piece == WPAWN ? 8 : -8)) * 12 + 
            (piece == WPAWN ? BPAWN : WPAWN)
          ];
      return;
    }
    if (m->flags & CASTLING_WQ) /* castling cases & respective rook mov     */
    {
      *z ^=
        zobrist [a1 * 12 + WROOK] ^
        zobrist [d1 * 12 + WROOK]; 
      return;
    }
    if (m->flags & CASTLING_WK)
    {
      *z ^=
        zobrist [h1 * 12 + WROOK] ^
        zobrist [f1 * 12 + WROOK];
      return;
    }
    if (m->flags & CASTLING_BQ)
    {
      *z ^=
        zobrist [a8 * 12 + BROOK] ^
        zobrist [d8 * 12 + BROOK];
      return;
    }
    if (m->flags & CASTLING_BK)
    {
      *z ^=
        zobrist [h8 * 12 + BROOK] ^
        zobrist [f8 * 12 + BROOK];
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

    if (move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
      npieces --; 
    color = !color;
    fullclock++;

    /* go one level deeper in the tree traversal */
    b[1].moveLoc = b[0].moveLoc + b[0].totalMoves;
    b = ++(*y);
    BoardAllMoves (b);

    return 1;
  }

  int BoardPrevLevel (_Board ** y)
  {
    _Board * b = --(*y);

    _Move * move = MOVES_AT (b);

    if (move->flags & (MOVE_CAPTURE | MOVE_ENP_CAPTURE))
      npieces ++; 
    color = !color;
    fullclock--;

    BoardUnmove (move);

    b [0].moveLoc ++;
    b [0].totalMoves --;
    movesall.len = (b[0].moveLoc + b[0].totalMoves) * sizeof (_Move);
    return 1;
  }

#endif
