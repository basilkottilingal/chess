#ifndef _CHESS_TREE_H_
#define _CHESS_TREE_H_

  #include "move.h"

  _Board * BoardRoot (const char * fen)
  {
    _Board * r = BoardSetFromFEN (fen);
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
